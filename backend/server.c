#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include <limits.h>
#include "json.h"
#include "organizer.h"

// Forward declarations
void handle_client(int client_fd);
void handle_api_request(int client_fd, const char *method, const char *path, const char *body);
void handle_static_request(int client_fd, const char *path);
void send_json(int client_fd, int status_code, const char *status_text, const char *body);
void send_404(int client_fd);

int is_local_env() {
    // If the user explicitly sets the env to web/cloud, respect it
    char *env = getenv("NEUROSORT_ENV");
    if (env && strcmp(env, "web") == 0) {
        return 0;
    }
    // If running in Vercel or other cloud environments, return False
    if (getenv("VERCEL") || getenv("NOW_BUILDER")) {
        return 0;
    }
    return 1;
}

// Client thread entry point
void *client_thread(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    handle_client(client_fd);
    close(client_fd);
    return NULL;
}

// Read and parse request
void handle_client(int client_fd) {
    char req_buf[32768];
    memset(req_buf, 0, sizeof(req_buf));
    
    // Read HTTP headers (until \r\n\r\n)
    int total_read = 0;
    while (total_read < (int)sizeof(req_buf) - 1) {
        int r = recv(client_fd, req_buf + total_read, 1, 0);
        if (r <= 0) break;
        total_read++;
        if (total_read >= 4 && strcmp(req_buf + total_read - 4, "\r\n\r\n") == 0) {
            break;
        }
    }
    req_buf[total_read] = '\0';
    
    if (total_read == 0) return;
    
    // Parse method and path
    char method[32] = "";
    char path[4096] = "";
    sscanf(req_buf, "%31s %4095s", method, path);
    
    // Handle CORS preflight options
    if (strcmp(method, "OPTIONS") == 0) {
        const char *header = 
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
            "Connection: close\r\n\r\n";
        send(client_fd, header, strlen(header), 0);
        return;
    }
    
    // Read POST body if Content-Length header is present
    int content_length = 0;
    char *cl_ptr = strcasestr(req_buf, "Content-Length:");
    if (cl_ptr) {
        cl_ptr += strlen("Content-Length:");
        content_length = atoi(cl_ptr);
    }
    
    char *body = NULL;
    if (content_length > 0 && content_length < 10 * 1024 * 1024) { // limit body size to 10MB
        body = malloc(content_length + 1);
        int body_read = 0;
        
        // Check if part of body was already read in req_buf
        char *body_start = strstr(req_buf, "\r\n\r\n");
        if (body_start) {
            body_start += 4;
            int already_read = (req_buf + total_read) - body_start;
            if (already_read > 0) {
                if (already_read > content_length) already_read = content_length;
                memcpy(body, body_start, already_read);
                body_read = already_read;
            }
        }
        
        while (body_read < content_length) {
            int r = recv(client_fd, body + body_read, content_length - body_read, 0);
            if (r <= 0) break;
            body_read += r;
        }
        body[body_read] = '\0';
    }
    
    // Route request
    if (strncmp(path, "/api/", 5) == 0 || 
        strcmp(path, "/config") == 0 || 
        strcmp(path, "/organize") == 0 || 
        strcmp(path, "/save-organized") == 0 || 
        strcmp(path, "/health") == 0) {
        handle_api_request(client_fd, method, path, body);
    } else {
        handle_static_request(client_fd, path);
    }
    
    if (body) free(body);
}

void send_json(int client_fd, int status_code, const char *status_text, const char *body) {
    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n",
        status_code, status_text, strlen(body));
    send(client_fd, header, strlen(header), 0);
    send(client_fd, body, strlen(body), 0);
}

void send_404(int client_fd) {
    const char *body = "{\"error\":\"Not Found\"}";
    send_json(client_fd, 404, "Not Found", body);
}

// macOS osascript folder/file pickers
void handle_select_files(int client_fd) {
    if (!is_local_env()) {
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"File picker is not supported in web mode. Please drag and drop or upload files directly.\"}");
        return;
    }
    
    const char *cmd = "osascript -e 'set selectedItems to choose file with prompt \"Select files for NeuroSort AI\" with multiple selections allowed' "
                      "-e 'set output to \"\"' "
                      "-e 'repeat with selectedItem in selectedItems' "
                      "-e 'set output to output & POSIX path of selectedItem & linefeed' "
                      "-e 'end repeat' "
                      "-e 'return output' 2>/dev/null";
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        send_json(client_fd, 500, "Internal Server Error", "{\"error\":\"Could not open the file picker.\"}");
        return;
    }
    
    char *paths_json = malloc(8192);
    strcpy(paths_json, "{\"paths\":[");
    size_t cap = 8192;
    size_t len = strlen(paths_json);
    
    char line[4096];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) == 0) continue;
        
        char *escaped = escape_json_string(line);
        size_t esc_len = strlen(escaped);
        
        // Ensure buffer has space (escaped + formatting characters)
        if (len + esc_len + 16 >= cap) {
            cap *= 2;
            paths_json = realloc(paths_json, cap);
        }
        
        if (count > 0) {
            strcat(paths_json + len, ",");
            len++;
        }
        
        sprintf(paths_json + len, "\"%s\"", escaped);
        len += strlen(paths_json + len);
        free(escaped);
        count++;
    }
    strcat(paths_json + len, "]}");
    
    int status = pclose(fp);
    if (status != 0 || count == 0) {
        free(paths_json);
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"Selection cancelled.\"}");
    } else {
        send_json(client_fd, 200, "OK", paths_json);
        free(paths_json);
    }
}

void handle_select_folders(int client_fd) {
    if (!is_local_env()) {
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"Folder picker is not supported in web mode. Please drag and drop or upload folders directly.\"}");
        return;
    }
    
    const char *cmd = "osascript -e 'set selectedItems to choose folder with prompt \"Select folders for NeuroSort AI\" with multiple selections allowed' "
                      "-e 'set output to \"\"' "
                      "-e 'repeat with selectedItem in selectedItems' "
                      "-e 'set output to output & POSIX path of selectedItem & linefeed' "
                      "-e 'end repeat' "
                      "-e 'return output' 2>/dev/null";
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        send_json(client_fd, 500, "Internal Server Error", "{\"error\":\"Could not open the folder picker.\"}");
        return;
    }
    
    char *paths_json = malloc(8192);
    strcpy(paths_json, "{\"paths\":[");
    size_t cap = 8192;
    size_t len = strlen(paths_json);
    
    char line[4096];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) == 0) continue;
        
        char *escaped = escape_json_string(line);
        size_t esc_len = strlen(escaped);
        
        if (len + esc_len + 16 >= cap) {
            cap *= 2;
            paths_json = realloc(paths_json, cap);
        }
        
        if (count > 0) {
            strcat(paths_json + len, ",");
            len++;
        }
        
        sprintf(paths_json + len, "\"%s\"", escaped);
        len += strlen(paths_json + len);
        free(escaped);
        count++;
    }
    strcat(paths_json + len, "]}");
    
    int status = pclose(fp);
    if (status != 0 || count == 0) {
        free(paths_json);
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"Selection cancelled.\"}");
    } else {
        send_json(client_fd, 200, "OK", paths_json);
        free(paths_json);
    }
}

void handle_select_destination(int client_fd) {
    if (!is_local_env()) {
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"Destination picker is not supported in web mode.\"}");
        return;
    }
    
    const char *cmd = "osascript -e 'POSIX path of (choose folder with prompt \"Select an output folder for NeuroSort AI\")' 2>/dev/null";
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        send_json(client_fd, 500, "Internal Server Error", "{\"error\":\"Could not open the destination picker.\"}");
        return;
    }
    
    char line[4096] = "";
    if (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
    }
    
    int status = pclose(fp);
    if (status != 0 || strlen(line) == 0) {
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"Selection cancelled.\"}");
    } else {
        char resp[8192];
        char *escaped = escape_json_string(line);
        snprintf(resp, sizeof(resp), "{\"path\":\"%s\"}", escaped);
        free(escaped);
        send_json(client_fd, 200, "OK", resp);
    }
}

// POST /api/organize
void handle_organize(int client_fd, const char *body) {
    if (!body) {
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"Empty body\"}");
        return;
    }
    
    json_value *root = json_parse(body);
    if (!root || root->type != JSON_OBJECT) {
        if (root) json_free(root);
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    json_value *files_metadata = json_obj_get(root, "files_metadata");
    json_value *sources_val = json_obj_get(root, "sources");
    json_value *dest_path_val = json_obj_get(root, "destinationPath");
    json_value *sort_by_val = json_obj_get(root, "sortBy");
    json_value *apply_changes_val = json_obj_get(root, "applyChanges");
    
    const char *sort_by = "name";
    if (sort_by_val && sort_by_val->type == JSON_STRING) {
        sort_by = json_as_string(sort_by_val);
    }
    
    int apply_changes = 0;
    if (apply_changes_val) {
        if (apply_changes_val->type == JSON_BOOL) {
            apply_changes = json_as_bool(apply_changes_val);
        } else if (apply_changes_val->type == JSON_NUMBER) {
            apply_changes = (int)json_as_number(apply_changes_val);
        }
    }
    
    const char *dest_path = NULL;
    if (dest_path_val && dest_path_val->type == JSON_STRING) {
        dest_path = json_as_string(dest_path_val);
    }
    
    char *resp_str = NULL;
    
    if (files_metadata && files_metadata->type == JSON_ARRAY) {
        // Web mode
        OrganizeResult *res = organize_metadata(files_metadata, sort_by);
        if (res) {
            resp_str = serialize_organize_result(res);
            free_organize_result(res);
        }
    } else {
        // Local mode
        if (!is_local_env()) {
            json_free(root);
            send_json(client_fd, 400, "Bad Request", "{\"error\":\"Local paths are not supported in web mode. Please upload files directly.\"}");
            return;
        }
        
        if (!sources_val || sources_val->type != JSON_ARRAY) {
            json_value *path_val = json_obj_get(root, "path");
            if (path_val && path_val->type == JSON_STRING) {
                char *srcs[1];
                srcs[0] = (char *)json_as_string(path_val);
                OrganizeResult *res = organize_sources(srcs, 1, sort_by, apply_changes, dest_path);
                if (res) {
                    resp_str = serialize_organize_result(res);
                    free_organize_result(res);
                }
            } else {
                json_free(root);
                send_json(client_fd, 400, "Bad Request", "{\"error\":\"Select at least one file or folder.\"}");
                return;
            }
        } else {
            int src_count = sources_val->u.array.count;
            if (src_count == 0) {
                json_free(root);
                send_json(client_fd, 400, "Bad Request", "{\"error\":\"Select at least one file or folder.\"}");
                return;
            }
            
            char **srcs = malloc(sizeof(char *) * src_count);
            for (int i = 0; i < src_count; i++) {
                srcs[i] = (char *)json_as_string(sources_val->u.array.items[i]);
            }
            
            OrganizeResult *res = organize_sources(srcs, src_count, sort_by, apply_changes, dest_path);
            free(srcs);
            if (res) {
                resp_str = serialize_organize_result(res);
                free_organize_result(res);
            }
        }
    }
    
    json_free(root);
    
    if (resp_str) {
        send_json(client_fd, 200, "OK", resp_str);
        free(resp_str);
    } else {
        send_json(client_fd, 500, "Internal Server Error", "{\"error\":\"An error occurred while organizing files.\"}");
    }
}

// POST /api/save-organized
void handle_save_organized(int client_fd, const char *body) {
    if (!is_local_env()) {
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"Saving locally is not supported in web mode. Files will be zipped and downloaded through your browser.\"}");
        return;
    }
    
    if (!body) {
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"Empty body\"}");
        return;
    }
    
    json_value *root = json_parse(body);
    if (!root || root->type != JSON_OBJECT) {
        if (root) json_free(root);
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    json_value *sources_val = json_obj_get(root, "sources");
    json_value *sort_by_val = json_obj_get(root, "sortBy");
    json_value *dest_path_val = json_obj_get(root, "destinationPath");
    json_value *save_mode_val = json_obj_get(root, "saveMode");
    
    const char *sort_by = "name";
    if (sort_by_val && sort_by_val->type == JSON_STRING) {
        sort_by = json_as_string(sort_by_val);
    }
    
    const char *dest_path = NULL;
    if (dest_path_val && dest_path_val->type == JSON_STRING) {
        dest_path = json_as_string(dest_path_val);
    }
    
    const char *save_mode = "downloads";
    if (save_mode_val && save_mode_val->type == JSON_STRING) {
        save_mode = json_as_string(save_mode_val);
    }
    
    if (!sources_val || sources_val->type != JSON_ARRAY || sources_val->u.array.count == 0) {
        json_free(root);
        send_json(client_fd, 400, "Bad Request", "{\"error\":\"Choose a save location first.\"}");
        return;
    }
    
    int src_count = sources_val->u.array.count;
    char **srcs = malloc(sizeof(char *) * src_count);
    for (int i = 0; i < src_count; i++) {
        srcs[i] = (char *)json_as_string(sources_val->u.array.items[i]);
    }
    
    char *resp_str = save_organized_copy(srcs, src_count, sort_by, dest_path, save_mode);
    free(srcs);
    json_free(root);
    
    if (resp_str) {
        send_json(client_fd, 200, "OK", resp_str);
        free(resp_str);
    } else {
        send_json(client_fd, 500, "Internal Server Error", "{\"error\":\"An error occurred while saving organized files.\"}");
    }
}

// API router
void handle_api_request(int client_fd, const char *method, const char *path, const char *body) {
    if (strcmp(path, "/api/config") == 0 || strcmp(path, "/config") == 0) {
        if (strcmp(method, "GET") == 0) {
            char resp[128];
            snprintf(resp, sizeof(resp), "{\"envMode\":\"%s\"}", is_local_env() ? "local" : "web");
            send_json(client_fd, 200, "OK", resp);
        } else {
            send_json(client_fd, 405, "Method Not Allowed", "{\"error\":\"Method Not Allowed\"}");
        }
    } else if (strcmp(path, "/api/select-files") == 0) {
        if (strcmp(method, "GET") == 0) {
            handle_select_files(client_fd);
        } else {
            send_json(client_fd, 405, "Method Not Allowed", "{\"error\":\"Method Not Allowed\"}");
        }
    } else if (strcmp(path, "/api/select-folders") == 0) {
        if (strcmp(method, "GET") == 0) {
            handle_select_folders(client_fd);
        } else {
            send_json(client_fd, 405, "Method Not Allowed", "{\"error\":\"Method Not Allowed\"}");
        }
    } else if (strcmp(path, "/api/select-destination") == 0 || strcmp(path, "/api/select-folder") == 0) {
        if (strcmp(method, "GET") == 0) {
            handle_select_destination(client_fd);
        } else {
            send_json(client_fd, 405, "Method Not Allowed", "{\"error\":\"Method Not Allowed\"}");
        }
    } else if (strcmp(path, "/api/organize") == 0 || strcmp(path, "/organize") == 0) {
        if (strcmp(method, "POST") == 0) {
            handle_organize(client_fd, body);
        } else {
            send_json(client_fd, 405, "Method Not Allowed", "{\"error\":\"Method Not Allowed\"}");
        }
    } else if (strcmp(path, "/api/save-organized") == 0 || strcmp(path, "/save-organized") == 0) {
        if (strcmp(method, "POST") == 0) {
            handle_save_organized(client_fd, body);
        } else {
            send_json(client_fd, 405, "Method Not Allowed", "{\"error\":\"Method Not Allowed\"}");
        }
    } else if (strcmp(path, "/health") == 0) {
        send_json(client_fd, 200, "OK", "{\"status\":\"ok\",\"app\":\"NeuroSort AI\"}");
    } else {
        send_404(client_fd);
    }
}

// Serve static assets from frontend/dist
void handle_static_request(int client_fd, const char *path) {
    char filepath[PATH_MAX];
    
    // Simple URL decoding (e.g. "%20" -> " ")
    char decoded_path[4096];
    int i = 0, j = 0;
    while (path[i] && j < (int)sizeof(decoded_path) - 1) {
        if (path[i] == '%' && path[i + 1] && path[i + 2]) {
            char hex[3] = {path[i + 1], path[i + 2], '\0'};
            decoded_path[j++] = (char)strtol(hex, NULL, 16);
            i += 3;
        } else {
            decoded_path[j++] = path[i++];
        }
    }
    decoded_path[j] = '\0';
    
    // Determine the base folder dynamically
    char prefix[256] = "frontend/dist";
    struct stat st_dir;
    if (stat("frontend/dist", &st_dir) == 0 && S_ISDIR(st_dir.st_mode)) {
        strcpy(prefix, "frontend/dist");
    } else if (stat("../frontend/dist", &st_dir) == 0 && S_ISDIR(st_dir.st_mode)) {
        strcpy(prefix, "../frontend/dist");
    }
    
    // Map URL path to file in prefix
    if (strcmp(decoded_path, "/") == 0 || strlen(decoded_path) == 0) {
        snprintf(filepath, sizeof(filepath), "%s/index.html", prefix);
    } else {
        const char *p = decoded_path;
        if (p[0] == '/') p++;
        snprintf(filepath, sizeof(filepath), "%s/%s", prefix, p);
    }
    
    // Check if file exists, if not fall back to index.html for React router support
    struct stat st;
    if (stat(filepath, &st) != 0 || !S_ISREG(st.st_mode)) {
        snprintf(filepath, sizeof(filepath), "%s/index.html", prefix);
        if (stat(filepath, &st) != 0) {
            send_404(client_fd);
            return;
        }
    }
    
    // Determine content type by extension
    const char *content_type = "application/octet-stream";
    const char *dot = strrchr(filepath, '.');
    if (dot) {
        if (strcasecmp(dot, ".html") == 0) content_type = "text/html";
        else if (strcasecmp(dot, ".css") == 0) content_type = "text/css";
        else if (strcasecmp(dot, ".js") == 0) content_type = "text/javascript";
        else if (strcasecmp(dot, ".json") == 0) content_type = "application/json";
        else if (strcasecmp(dot, ".png") == 0) content_type = "image/png";
        else if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0) content_type = "image/jpeg";
        else if (strcasecmp(dot, ".svg") == 0) content_type = "image/svg+xml";
        else if (strcasecmp(dot, ".gif") == 0) content_type = "image/gif";
        else if (strcasecmp(dot, ".webp") == 0) content_type = "image/webp";
    }
    
    // Serve file
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        send_404(client_fd);
        return;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n",
        content_type, size);
    send(client_fd, header, strlen(header), 0);
    
    char buf[16384];
    size_t read_bytes;
    while ((read_bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
        send(client_fd, buf, read_bytes, 0);
    }
    fclose(f);
}

int main() {
    int port = 5050;
    char *port_env = getenv("PORT");
    if (port_env) {
        port = atoi(port_env);
    }
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return 1;
    }
    
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }
    
    if (listen(server_fd, 100) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }
    
    printf("NeuroSort AI C Backend running on port %d\n", port);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }
        
        pthread_t thread;
        int *client_fd_ptr = malloc(sizeof(int));
        *client_fd_ptr = client_fd;
        if (pthread_create(&thread, NULL, client_thread, client_fd_ptr) != 0) {
            perror("Thread creation failed");
            close(client_fd);
            free(client_fd_ptr);
        } else {
            pthread_detach(thread);
        }
    }
    
    close(server_fd);
    return 0;
}
