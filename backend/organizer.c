#include "organizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>

// Categories
static const char *CATEGORY_STUDY = "Study";
static const char *CATEGORY_IMAGES = "Images";
static const char *CATEGORY_DOCUMENTS = "Documents";
static const char *CATEGORY_VIDEOS = "Videos";
static const char *CATEGORY_OTHERS = "Others";

static const char *FOLDER_STUDY = "Study Hub";
static const char *FOLDER_IMAGES = "Images";
static const char *FOLDER_DOCUMENTS = "Documents";
static const char *FOLDER_VIDEOS = "Videos";
static const char *FOLDER_OTHERS = "Others";

static const char *get_folder_for_category(const char *category) {
    if (strcmp(category, CATEGORY_STUDY) == 0) return FOLDER_STUDY;
    if (strcmp(category, CATEGORY_IMAGES) == 0) return FOLDER_IMAGES;
    if (strcmp(category, CATEGORY_DOCUMENTS) == 0) return FOLDER_DOCUMENTS;
    if (strcmp(category, CATEGORY_VIDEOS) == 0) return FOLDER_VIDEOS;
    return FOLDER_OTHERS;
}

// Extensions checking
static const char *IMAGE_EXTS[] = {".jpg", ".jpeg", ".png", ".gif", ".webp", ".heic", ".svg"};
static const char *DOC_EXTS[] = {".pdf", ".docx", ".doc", ".txt", ".pptx", ".xlsx", ".csv"};
static const char *VIDEO_EXTS[] = {".mp4", ".mkv", ".mov", ".avi", ".webm"};

static int has_extension(const char *name, const char **exts, int count) {
    size_t len = strlen(name);
    for (int i = 0; i < count; i++) {
        size_t ext_len = strlen(exts[i]);
        if (len >= ext_len && strcasecmp(name + len - ext_len, exts[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static const char *classify_file(const char *name) {
    char lower_name[1024];
    size_t len = strlen(name);
    if (len >= sizeof(lower_name)) len = sizeof(lower_name) - 1;
    for (size_t i = 0; i < len; i++) {
        lower_name[i] = (name[i] >= 'A' && name[i] <= 'Z') ? (name[i] + 32) : name[i];
    }
    lower_name[len] = '\0';

    if (strstr(lower_name, "notes") != NULL || strstr(lower_name, "assignment") != NULL) {
        return CATEGORY_STUDY;
    }
    if (has_extension(lower_name, (const char *[]){".jpg", ".png"}, 2)) {
        return CATEGORY_IMAGES;
    }
    if (has_extension(lower_name, (const char *[]){".pdf", ".docx"}, 2)) {
        return CATEGORY_DOCUMENTS;
    }
    if (has_extension(lower_name, (const char *[]){".mp4", ".mkv"}, 2)) {
        return CATEGORY_VIDEOS;
    }

    // Extra extensions check
    if (has_extension(lower_name, IMAGE_EXTS, sizeof(IMAGE_EXTS)/sizeof(IMAGE_EXTS[0]))) {
        return CATEGORY_IMAGES;
    }
    if (has_extension(lower_name, DOC_EXTS, sizeof(DOC_EXTS)/sizeof(DOC_EXTS[0]))) {
        return CATEGORY_DOCUMENTS;
    }
    if (has_extension(lower_name, VIDEO_EXTS, sizeof(VIDEO_EXTS)/sizeof(VIDEO_EXTS[0]))) {
        return CATEGORY_VIDEOS;
    }

    return CATEGORY_OTHERS;
}

static const char *get_priority(const char *name) {
    char lower_name[1024];
    size_t len = strlen(name);
    if (len >= sizeof(lower_name)) len = sizeof(lower_name) - 1;
    for (size_t i = 0; i < len; i++) {
        lower_name[i] = (name[i] >= 'A' && name[i] <= 'Z') ? (name[i] + 32) : name[i];
    }
    lower_name[len] = '\0';

    if (strstr(lower_name, "final") != NULL || strstr(lower_name, "exam") != NULL) {
        return "High";
    }
    if (strstr(lower_name, "notes") != NULL) {
        return "Medium";
    }
    return "Low";
}

// Ignored files/directories
static int is_ignored_directory(const char *name) {
    if (name[0] == '.') return 1;
    const char *ignored[] = {
        "dist", "node_modules", "NeuroSort Organized",
        "Study Hub", "Images", "Documents", "Videos", "Others"
    };
    if (strcmp(name, ".git") == 0 || strcmp(name, ".venv") == 0 || strcmp(name, "__pycache__") == 0) {
        return 1;
    }
    for (size_t i = 0; i < sizeof(ignored)/sizeof(ignored[0]); i++) {
        if (strcmp(name, ignored[i]) == 0) return 1;
    }
    return 0;
}

static int is_ignored_file(const char *name) {
    if (name[0] == '.') return 1;
    if (strcmp(name, ".DS_Store") == 0 || strcmp(name, "Thumbs.db") == 0) {
        return 1;
    }
    return 0;
}

// Helpers
static void get_file_type(const char *name, char *out_type, size_t max_len) {
    const char *dot = strrchr(name, '.');
    if (dot) {
        size_t len = strlen(dot);
        if (len >= max_len) len = max_len - 1;
        for (size_t i = 0; i < len; i++) {
            out_type[i] = (dot[i] >= 'A' && dot[i] <= 'Z') ? (dot[i] + 32) : dot[i];
        }
        out_type[len] = '\0';
    } else {
        strncpy(out_type, "no extension", max_len);
    }
}

// Dynamic StringBuilder
typedef struct {
    char *buf;
    size_t size;
    size_t capacity;
} StringBuilder;

static void sb_init(StringBuilder *sb) {
    sb->capacity = 16384;
    sb->buf = malloc(sb->capacity);
    sb->buf[0] = '\0';
    sb->size = 0;
}

static void sb_append(StringBuilder *sb, const char *str) {
    size_t len = strlen(str);
    if (sb->size + len >= sb->capacity) {
        while (sb->size + len >= sb->capacity) {
            sb->capacity *= 2;
        }
        sb->buf = realloc(sb->buf, sb->capacity);
    }
    strcpy(sb->buf + sb->size, str);
    sb->size += len;
}

static void sb_free(StringBuilder *sb) {
    free(sb->buf);
}

// Custom mergesort implementation for DSA requirement (stable sort)
static int compare_records(const FileRecord *a, const FileRecord *b, const char *sort_by) {
    if (strcmp(sort_by, "type") == 0) {
        int cmp_type = strcasecmp(a->type, b->type);
        if (cmp_type != 0) return cmp_type;
    }
    int cmp_name = strcasecmp(a->name, b->name);
    if (cmp_name != 0) return cmp_name;
    return strcasecmp(a->source, b->source);
}

static void merge(FileRecord **arr, int l, int m, int r, const char *sort_by) {
    int n1 = m - l + 1;
    int n2 = r - m;
    FileRecord **L = malloc(sizeof(FileRecord*) * n1);
    FileRecord **R = malloc(sizeof(FileRecord*) * n2);
    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int j = 0; j < n2; j++) R[j] = arr[m + 1 + j];
    
    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        if (compare_records(L[i], R[j], sort_by) <= 0) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    free(L);
    free(R);
}

static void merge_sort(FileRecord **arr, int l, int r, const char *sort_by) {
    if (l < r) {
        int m = l + (r - l) / 2;
        merge_sort(arr, l, m, sort_by);
        merge_sort(arr, m + 1, r, sort_by);
        merge(arr, l, m, r, sort_by);
    }
}

// Scanning directory recursively
static void scan_directory(const char *dir_path, char ***collected, int *count, char ***seen_real, int *seen_count) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (lstat(full_path, &st) != 0) {
            continue;
        }

        // Get canonical/real path
        char real_p[PATH_MAX];
        char *resolved = realpath(full_path, real_p);
        if (!resolved) {
            // If realpath fails (e.g. symlinks, permissions, or missing parent), fallback to full_path
            strncpy(real_p, full_path, PATH_MAX - 1);
            real_p[PATH_MAX - 1] = '\0';
        }

        // Check if real path is already scanned
        int already_seen = 0;
        for (int i = 0; i < *seen_count; i++) {
            if (strcmp((*seen_real)[i], real_p) == 0) {
                already_seen = 1;
                break;
            }
        }
        if (already_seen) continue;

        if (S_ISDIR(st.st_mode)) {
            if (is_ignored_directory(entry->d_name)) {
                continue;
            }
            // Add directory to seen list to prevent circular traversal
            *seen_real = realloc(*seen_real, sizeof(char*) * (*seen_count + 1));
            (*seen_real)[*seen_count] = strdup(real_p);
            (*seen_count)++;

            scan_directory(full_path, collected, count, seen_real, seen_count);
        } else if (S_ISREG(st.st_mode)) {
            if (is_ignored_file(entry->d_name)) {
                continue;
            }
            // Add file realpath to seen list
            *seen_real = realloc(*seen_real, sizeof(char*) * (*seen_count + 1));
            (*seen_real)[*seen_count] = strdup(real_p);
            (*seen_count)++;

            // Collect file
            *collected = realloc(*collected, sizeof(char*) * (*count + 1));
            (*collected)[*count] = strdup(full_path);
            (*count)++;
        }
    }
    closedir(dir);
}

// Build FileRecord
static FileRecord *build_record(const char *source_path, int duplicate_count, time_t custom_mtime, long long custom_size) {
    FileRecord *rec = calloc(1, sizeof(FileRecord));
    
    // Get basename
    const char *base = strrchr(source_path, '/');
    if (base) base++;
    else base = source_path;
    
    rec->name = strdup(base);
    rec->source = strdup(source_path);
    
    char type_buf[64];
    get_file_type(base, type_buf, sizeof(type_buf));
    rec->type = strdup(type_buf);
    
    rec->category = strdup(classify_file(base));
    rec->priority = strdup(get_priority(base));
    rec->folder = strdup(get_folder_for_category(rec->category));
    
    time_t mtime = 0;
    long long fsize = 0;
    
    if (custom_mtime != 0) {
        mtime = custom_mtime;
        fsize = custom_size;
    } else {
        struct stat st;
        if (stat(source_path, &st) == 0) {
            mtime = st.st_mtime;
            fsize = st.st_size;
        } else {
            mtime = time(NULL);
            fsize = 0;
        }
    }
    
    rec->size_bytes = fsize;
    
    // Format modified_at
    char time_buf[64];
    struct tm *tm_info = localtime(&mtime);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", tm_info);
    rec->modified_at = strdup(time_buf);
    
    // Smart Tags
    char *tags[10];
    int t_count = 0;
    
    char lower_name[1024];
    size_t len = strlen(base);
    if (len >= sizeof(lower_name)) len = sizeof(lower_name) - 1;
    for (size_t i = 0; i < len; i++) {
        lower_name[i] = (base[i] >= 'A' && base[i] <= 'Z') ? (base[i] + 32) : base[i];
    }
    lower_name[len] = '\0';
    
    if (strcmp(rec->category, CATEGORY_STUDY) == 0) {
        tags[t_count++] = "academic";
    }
    if (strcmp(rec->priority, "High") == 0) {
        tags[t_count++] = "exam-ready";
    }
    if (duplicate_count > 1) {
        tags[t_count++] = "duplicate-candidate";
    }
    
    // cleanup-candidate if older than 180 days (180 * 24 * 3600 seconds)
    time_t cutoff = time(NULL) - (180 * 24 * 3600);
    if (mtime < cutoff) {
        tags[t_count++] = "cleanup-candidate";
    }
    
    if (strstr(lower_name, "project") != NULL || strstr(lower_name, "report") != NULL) {
        tags[t_count++] = "project-work";
    }
    if (strcmp(rec->category, CATEGORY_IMAGES) == 0 || strcmp(rec->category, CATEGORY_VIDEOS) == 0) {
        tags[t_count++] = "media";
    }
    
    if (t_count == 0) {
        tags[t_count++] = "standard";
    }
    
    rec->smart_tags = malloc(sizeof(char*) * t_count);
    for (int i = 0; i < t_count; i++) {
        rec->smart_tags[i] = strdup(tags[i]);
    }
    rec->smart_tags_count = t_count;
    
    // ============================================================
    // PRIORITY SCORING ENGINE — Comprehensive Keyword Database
    // s0ucipher uses public knowledge of document sensitivity.
    // ============================================================
    int base_min = 0, base_max = 30;

    // ---------------------------------------------------------------
    // TIER 1 — VERY HIGH (90-100%)
    // Government & National IDs, Critical Legal, Sensitive Personal
    // Based on global standards: GDPR, CCPA, RBI, MCA document classes
    // ---------------------------------------------------------------
    const char *tier_very_high[] = {
        // Government Identity Documents
        "passport", "aadhar", "aadhaar", "pan card", "pancard",
        "driving licence", "driving license", "driverlicense",
        "voter id", "voterid", "voter card", "national id",
        "ssn", "social security", "ration card",
        "visa", "work permit", "residence permit",
        // Academic — Marksheets, Results, Certificates (VERY important)
        "marksheet", "mark sheet", "markcard", "grade card",
        "result", "scorecard", "score card",
        "admit card", "admitcard", "hall ticket", "hallticket",
        "registration", "enrollment", "enrolment", "roll number",
        "degree certificate", "provisional certificate",
        "migration certificate", "character certificate",
        "passing certificate", "passing cert",
        "bonafide", "transfer certificate", "tc ",
        // Government Issued Tax & Finance
        "tax return", "itr", "form16", "form 16",
        "gst", "tds certificate", "income tax",
        "w2", "w-2", "1099", "tax invoice",
        // Critical Legal Documents
        "birth certificate", "death certificate",
        "marriage certificate", "divorce", "adoption",
        "property deed", "land deed", "sale deed",
        "power of attorney", "affidavit", "will ",
        "agreement", "legal notice", "court order",
        "fir", "police report", "summons"
    };
    int tier_very_high_count = (int)(sizeof(tier_very_high) / sizeof(tier_very_high[0]));

    // ---------------------------------------------------------------
    // TIER 2 — HIGH (80-90%)
    // Financial Records, Medical, Professional Credentials
    // ---------------------------------------------------------------
    const char *tier_high[] = {
        // Banking & Finance
        "bank statement", "bank account", "account statement",
        "passbook", "cheque", "check", "demand draft",
        "invoice", "receipt", "billing", "bill ",
        "salary", "payslip", "pay slip", "payroll",
        "credit card", "loan", "mortgage", "emi statement",
        "insurance policy", "insurance", "premium",
        "mutual fund", "portfolio", "investment",
        "stock", "demat", "trading",
        "balance sheet", "profit loss", "financial statement",
        // Medical & Health
        "medical", "prescription", "medicine",
        "health report", "lab report", "pathology",
        "diagnosis", "discharge summary", "hospital",
        "vaccination", "vaccine", "immunization",
        "blood report", "xray", "x-ray", "mri", "scan report",
        // Professional Credentials
        "resume", "cv ", "curriculum vitae",
        "offer letter", "appointment letter", "experience letter",
        "relieving letter", "joining letter",
        "noc", "no objection", "recommendation",
        "certificate", "certification", "diploma",
        "transcript", "academic record",
        // Exam & Education High-Stakes
        "exam result", "examination", "final exam",
        "semester", "annual result"
    };
    int tier_high_count = (int)(sizeof(tier_high) / sizeof(tier_high[0]));

    // ---------------------------------------------------------------
    // TIER 3 — MEDIUM (50-80%)
    // Work & Study material, Projects, Reports, Assignments
    // ---------------------------------------------------------------
    const char *tier_medium[] = {
        // Academic Work
        "notes", "lecture", "handout", "syllabus",
        "textbook", "chapter", "assignment", "homework",
        "thesis", "dissertation", "research", "paper",
        "study", "revision", "summary",
        // Work & Projects
        "project", "report", "presentation", "proposal",
        "draft", "plan", "roadmap", "specification",
        "design", "architecture", "documentation",
        "minutes", "agenda", "meeting notes",
        "progress", "update", "status report",
        "contract draft", "quotation", "estimate",
        // Creative & Portfolio
        "portfolio", "creative", "artwork", "design work",
        "script", "manuscript", "storyboard"
    };
    int tier_medium_count = (int)(sizeof(tier_medium) / sizeof(tier_medium[0]));

    // ---------------------------------------------------------------
    // TIER 4 — LOW (30-50%)
    // Generic Documents without critical keywords
    // ---------------------------------------------------------------
    // (Handled by category fallback — CATEGORY_DOCUMENTS)

    // ---------------------------------------------------------------
    // TIER 5 — VERY LOW (0-30%)
    // Generic media files: images, videos, audio, memes, etc.
    // ---------------------------------------------------------------
    // (Handled by category fallback — CATEGORY_IMAGES / CATEGORY_VIDEOS)

    // ---------------------------------------------------------------
    // MATCH PHASE — Priority ordered (Tier 1 first, never overridden)
    // ---------------------------------------------------------------
    int matched_tier = 0; // 1=VeryHigh 2=High 3=Medium 4=Low 5=VeryLow

    for (int i = 0; i < tier_very_high_count; i++) {
        if (strstr(lower_name, tier_very_high[i])) { matched_tier = 1; break; }
    }
    if (!matched_tier) {
        for (int i = 0; i < tier_high_count; i++) {
            if (strstr(lower_name, tier_high[i])) { matched_tier = 2; break; }
        }
    }
    if (!matched_tier) {
        for (int i = 0; i < tier_medium_count; i++) {
            if (strstr(lower_name, tier_medium[i])) { matched_tier = 3; break; }
        }
    }
    // Category fallback if no keyword matched
    if (!matched_tier) {
        if (strcmp(rec->category, CATEGORY_DOCUMENTS) == 0) {
            matched_tier = 4;
        } else {
            matched_tier = 5;
        }
    }

    // Assign base ranges
    switch (matched_tier) {
        case 1: base_min = 90; base_max = 100; break;
        case 2: base_min = 80; base_max = 90;  break;
        case 3: base_min = 50; base_max = 80;  break;
        case 4: base_min = 30; base_max = 50;  break;
        default: base_min = 0; base_max = 30;  break;
    }

    // Hash-based deterministic variance — ensures natural spread within the tier
    unsigned int hash = 0;
    for (size_t i = 0; i < len; i++) {
        hash = hash * 31 + (unsigned char)lower_name[i];
    }
    hash += (unsigned int)(rec->size_bytes & 0xFFFFFFFF);

    int range = base_max - base_min;
    if (range <= 0) range = 1;
    int variance = (int)(hash % (unsigned int)range);
    double score = (double)(base_min + variance) / 100.0;
    if (score > 1.0) score = 1.0;
    if (score < 0.0) score = 0.0;
    rec->confidence = score;

    // Map score to priority label + AI explanation
    free(rec->priority);
    char reason_buf[1024];
    if (score <= 0.30) {
        rec->priority = strdup("Very Low");
        strcpy(reason_buf,
            "s0ucipher rated this Very Low (0-30%): generic media or unclassified file with no sensitive keywords detected.");
    } else if (score <= 0.50) {
        rec->priority = strdup("Low");
        strcpy(reason_buf,
            "s0ucipher rated this Low (30-50%): standard document without specific sensitive identifiers.");
    } else if (score <= 0.80) {
        rec->priority = strdup("Medium");
        strcpy(reason_buf,
            "s0ucipher rated this Medium (50-80%): academic notes, project work, or general study material detected.");
    } else if (score <= 0.90) {
        rec->priority = strdup("High");
        strcpy(reason_buf,
            "s0ucipher rated this High (80-90%): financial record, medical data, professional certificate, or official academic document detected.");
    } else {
        rec->priority = strdup("Very High");
        strcpy(reason_buf,
            "s0ucipher rated this Very High (90-100%): critical government ID, marksheet, result, registration, admit card, legal deed, or tax document detected.");
    }
    rec->ai_reason = strdup(reason_buf);
    
    // Suggested name
    char sug_buf[1024];
    // Strip extension from base name
    char base_no_ext[512];
    strcpy(base_no_ext, base);
    char *dot = strrchr(base_no_ext, '.');
    char ext_part[128] = "";
    if (dot) {
        strcpy(ext_part, dot);
        *dot = '\0';
    }
    
    // Replace spaces with underscores
    for (char *c = base_no_ext; *c; c++) {
        if (*c == ' ') *c = '_';
    }
    
    if (strcmp(rec->priority, "High") == 0) {
        snprintf(sug_buf, sizeof(sug_buf), "Important_%s%s", base_no_ext, ext_part);
    } else if (strcmp(rec->category, CATEGORY_STUDY) == 0) {
        snprintf(sug_buf, sizeof(sug_buf), "Study_%s%s", base_no_ext, ext_part);
    } else {
        snprintf(sug_buf, sizeof(sug_buf), "%s_%s%s", rec->category, base_no_ext, ext_part);
    }
    rec->suggested_name = strdup(sug_buf);
    
    return rec;
}

static void free_record(FileRecord *rec) {
    if (!rec) return;
    free(rec->name);
    free(rec->source);
    free(rec->type);
    free(rec->category);
    free(rec->priority);
    free(rec->folder);
    free(rec->modified_at);
    for (int i = 0; i < rec->smart_tags_count; i++) {
        free(rec->smart_tags[i]);
    }
    free(rec->smart_tags);
    free(rec->ai_reason);
    free(rec->suggested_name);
    free(rec);
}

// Destination path resolver with indexing
void get_safe_destination(const char *destination_root, const char *folder_name, const char *file_name, char *out_dir, char *out_dest, size_t max_len) {
    snprintf(out_dir, max_len, "%s/%s", destination_root, folder_name);
    snprintf(out_dest, max_len, "%s/%s", out_dir, file_name);

    struct stat st;
    if (stat(out_dest, &st) != 0) {
        // Safe, doesn't exist
        return;
    }

    // Exists, find extension
    char base[PATH_MAX];
    snprintf(base, sizeof(base), "%s", out_dest);
    char *dot = strrchr(base, '.');
    char ext[256] = "";
    if (dot) {
        char *slash = strrchr(base, '/');
        if (!slash || dot > slash) {
            strcpy(ext, dot);
            *dot = '\0';
        }
    }

    int index = 1;
    while (1) {
        snprintf(out_dest, max_len, "%s (%d)%s", base, index, ext);
        if (stat(out_dest, &st) != 0) {
            break;
        }
        index++;
    }
}

// Core implementation
OrganizeResult *organize_sources(char **sources, int sources_count, const char *sort_by, int apply_changes, const char *destination_path) {
    OrganizeResult *res = calloc(1, sizeof(OrganizeResult));
    
    // Normalize sources & scan
    char **collected_files = NULL;
    int files_count = 0;
    
    char **seen_real = NULL;
    int seen_count = 0;
    
    // Normalize paths
    char **norm_sources = malloc(sizeof(char*) * sources_count);
    int valid_sources_count = 0;
    
    for (int i = 0; i < sources_count; i++) {
        if (!sources[i] || strlen(sources[i]) == 0) continue;
        char real_p[PATH_MAX];
        char *resolved = realpath(sources[i], real_p);
        if (resolved) {
            norm_sources[valid_sources_count++] = strdup(real_p);
        } else {
            // Path doesn't exist or is invalid, just absolute it
            norm_sources[valid_sources_count++] = strdup(sources[i]);
        }
    }
    
    res->sources = norm_sources;
    res->sources_count = valid_sources_count;
    
    for (int i = 0; i < valid_sources_count; i++) {
        struct stat st;
        if (stat(norm_sources[i], &st) != 0) {
            continue;
        }
        
        char real_p[PATH_MAX];
        char *resolved = realpath(norm_sources[i], real_p);
        if (!resolved) {
            strncpy(real_p, norm_sources[i], PATH_MAX - 1);
            real_p[PATH_MAX - 1] = '\0';
        }
        
        // Check if already seen
        int already_seen = 0;
        for (int k = 0; k < seen_count; k++) {
            if (strcmp(seen_real[k], real_p) == 0) {
                already_seen = 1;
                break;
            }
        }
        if (already_seen) continue;
        
        if (S_ISDIR(st.st_mode)) {
            // Add directory to seen list
            seen_real = realloc(seen_real, sizeof(char*) * (seen_count + 1));
            seen_real[seen_count++] = strdup(real_p);
            
            scan_directory(norm_sources[i], &collected_files, &files_count, &seen_real, &seen_count);
        } else if (S_ISREG(st.st_mode)) {
            // Check ignored name
            const char *base = strrchr(norm_sources[i], '/');
            if (base) base++;
            else base = norm_sources[i];
            
            if (is_ignored_file(base)) continue;
            
            // Add file to seen list
            seen_real = realloc(seen_real, sizeof(char*) * (seen_count + 1));
            seen_real[seen_count++] = strdup(real_p);
            
            collected_files = realloc(collected_files, sizeof(char*) * (files_count + 1));
            collected_files[files_count++] = strdup(norm_sources[i]);
        }
    }
    
    // Free seen list
    for (int i = 0; i < seen_count; i++) free(seen_real[i]);
    free(seen_real);
    
    // Determine default destination root
    char dest_root[PATH_MAX];
    if (destination_path && strlen(destination_path) > 0) {
        strcpy(dest_root, destination_path);
    } else {
        // Default destination is: parent dir of first source / NeuroSort Organized
        if (valid_sources_count > 0) {
            char temp[PATH_MAX];
            strcpy(temp, norm_sources[0]);
            
            struct stat st;
            if (stat(temp, &st) == 0 && S_ISDIR(st.st_mode)) {
                // If it is a folder and it is the ONLY source, use it directly
                if (valid_sources_count == 1) {
                    strcpy(dest_root, temp);
                } else {
                    snprintf(dest_root, sizeof(dest_root), "%s/NeuroSort Organized", temp);
                }
            } else {
                char *slash = strrchr(temp, '/');
                if (slash) {
                    *slash = '\0';
                    snprintf(dest_root, sizeof(dest_root), "%s/NeuroSort Organized", temp);
                } else {
                    strcpy(dest_root, "./NeuroSort Organized");
                }
            }
        } else {
            strcpy(dest_root, "./NeuroSort Organized");
        }
    }
    
    res->path = strdup(dest_root);
    res->destination_path = strdup(dest_root);
    res->sort_by = strdup(sort_by);
    res->applied_changes = apply_changes;
    
    // Count duplicates by filename case-insensitively
    // We will build a helper count array
    int *dup_counts = calloc(files_count, sizeof(int));
    for (int i = 0; i < files_count; i++) {
        const char *base_i = strrchr(collected_files[i], '/');
        if (base_i) base_i++;
        else base_i = collected_files[i];
        
        int match_count = 0;
        for (int j = 0; j < files_count; j++) {
            const char *base_j = strrchr(collected_files[j], '/');
            if (base_j) base_j++;
            else base_j = collected_files[j];
            
            if (strcasecmp(base_i, base_j) == 0) {
                match_count++;
            }
        }
        dup_counts[i] = match_count;
    }
    
    // Build records
    res->before_count = files_count;
    res->before = malloc(sizeof(FileRecord*) * files_count);
    for (int i = 0; i < files_count; i++) {
        res->before[i] = build_record(collected_files[i], dup_counts[i], 0, 0);
        free(collected_files[i]);
    }
    free(collected_files);
    free(dup_counts);
    
    // Sort records using custom mergesort
    if (files_count > 0) {
        merge_sort(res->before, 0, files_count - 1, sort_by);
    }
    
    // Group duplicates
    // Collect unique duplicate group structures
    res->duplicates = NULL;
    res->duplicates_count = 0;
    
    int *grouped = calloc(files_count, sizeof(int));
    for (int i = 0; i < files_count; i++) {
        if (grouped[i]) continue;
        
        // Find other copies
        int count_copies = 0;
        for (int j = i; j < files_count; j++) {
            if (strcasecmp(res->before[i]->name, res->before[j]->name) == 0) {
                count_copies++;
            }
        }
        
        if (count_copies > 1) {
            DuplicateGroup *dg = malloc(sizeof(DuplicateGroup));
            dg->name = strdup(res->before[i]->name);
            dg->count = count_copies;
            dg->files = malloc(sizeof(FileRecord*) * count_copies);
            
            int idx = 0;
            for (int j = i; j < files_count; j++) {
                if (strcasecmp(res->before[i]->name, res->before[j]->name) == 0) {
                    dg->files[idx++] = res->before[j];
                    grouped[j] = 1;
                }
            }
            
            res->duplicates = realloc(res->duplicates, sizeof(DuplicateGroup*) * (res->duplicates_count + 1));
            res->duplicates[res->duplicates_count++] = dg;
        }
    }
    free(grouped);
    
    // Collect study files, important files, cleanup suggestions
    res->study_files = NULL;
    res->study_files_count = 0;
    
    res->important_files = NULL;
    res->important_files_count = 0;
    
    res->cleanup_suggestions = NULL;
    res->cleanup_suggestions_count = 0;
    
    res->moved_files = NULL;
    res->moved_files_count = 0;
    
    for (int i = 0; i < files_count; i++) {
        FileRecord *rec = res->before[i];
        
        if (strcmp(rec->category, CATEGORY_STUDY) == 0) {
            res->study_files = realloc(res->study_files, sizeof(FileRecord*) * (res->study_files_count + 1));
            res->study_files[res->study_files_count++] = rec;
        }
        
        if (strcmp(rec->priority, "High") == 0) {
            res->important_files = realloc(res->important_files, sizeof(FileRecord*) * (res->important_files_count + 1));
            res->important_files[res->important_files_count++] = rec;
        }
        
        // Smart tag cleanup check
        int has_dup = 0;
        int has_old = 0;
        for (int k = 0; k < rec->smart_tags_count; k++) {
            if (strcmp(rec->smart_tags[k], "duplicate-candidate") == 0) has_dup = 1;
            if (strcmp(rec->smart_tags[k], "cleanup-candidate") == 0) has_old = 1;
        }
        
        if (has_dup) {
            CleanupSuggestion *cs = malloc(sizeof(CleanupSuggestion));
            cs->file = strdup(rec->name);
            cs->source = strdup(rec->source);
            cs->reason = strdup("Possible duplicate name across selected sources. Review before deleting.");
            
            res->cleanup_suggestions = realloc(res->cleanup_suggestions, sizeof(CleanupSuggestion*) * (res->cleanup_suggestions_count + 1));
            res->cleanup_suggestions[res->cleanup_suggestions_count++] = cs;
        }
        
        if (has_old) {
            CleanupSuggestion *cs = malloc(sizeof(CleanupSuggestion));
            cs->file = strdup(rec->name);
            cs->source = strdup(rec->source);
            cs->reason = strdup("Old file. Consider archiving or cleaning it up.");
            
            res->cleanup_suggestions = realloc(res->cleanup_suggestions, sizeof(CleanupSuggestion*) * (res->cleanup_suggestions_count + 1));
            res->cleanup_suggestions[res->cleanup_suggestions_count++] = cs;
        }
        
        // Execute moves if apply_changes is true
        if (apply_changes) {
            char target_dir[PATH_MAX];
            char target_dest[PATH_MAX];
            get_safe_destination(dest_root, rec->folder, rec->name, target_dir, target_dest, sizeof(target_dest));
            
            mkdir_p(target_dir);
            if (move_file(rec->source, target_dest) == 0) {
                MovedFile *mf = malloc(sizeof(MovedFile));
                mf->from = strdup(rec->source);
                mf->to = strdup(target_dest);
                
                res->moved_files = realloc(res->moved_files, sizeof(MovedFile*) * (res->moved_files_count + 1));
                res->moved_files[res->moved_files_count++] = mf;
            }
        }
    }
    
    // Count unique categories present
    int cats_present[5] = {0};
    for (int i = 0; i < files_count; i++) {
        if (strcmp(res->before[i]->category, CATEGORY_STUDY) == 0) cats_present[0] = 1;
        else if (strcmp(res->before[i]->category, CATEGORY_IMAGES) == 0) cats_present[1] = 1;
        else if (strcmp(res->before[i]->category, CATEGORY_DOCUMENTS) == 0) cats_present[2] = 1;
        else if (strcmp(res->before[i]->category, CATEGORY_VIDEOS) == 0) cats_present[3] = 1;
        else cats_present[4] = 1;
    }
    int cats_count = 0;
    for (int i = 0; i < 5; i++) cats_count += cats_present[i];
    
    // Fill Dashboard
    res->dashboard.total_files = files_count;
    res->dashboard.source_count = valid_sources_count;
    res->dashboard.total_categories = cats_count;
    res->dashboard.important_files = res->important_files_count;
    res->dashboard.study_files = res->study_files_count;
    res->dashboard.duplicate_groups = res->duplicates_count;
    res->dashboard.cleanup_items = res->cleanup_suggestions_count;
    
    // Build Assistant
    res->assistant.name = strdup("s0ucipher");
    
    // strongest category count
    int max_cat_idx = 4;
    int cat_counts[5] = {0};
    for (int i = 0; i < files_count; i++) {
        if (strcmp(res->before[i]->category, CATEGORY_STUDY) == 0) cat_counts[0]++;
        else if (strcmp(res->before[i]->category, CATEGORY_IMAGES) == 0) cat_counts[1]++;
        else if (strcmp(res->before[i]->category, CATEGORY_DOCUMENTS) == 0) cat_counts[2]++;
        else if (strcmp(res->before[i]->category, CATEGORY_VIDEOS) == 0) cat_counts[3]++;
        else cat_counts[4]++;
    }
    int max_val = -1;
    for (int i = 0; i < 5; i++) {
        if (cat_counts[i] > max_val) {
            max_val = cat_counts[i];
            max_cat_idx = i;
        }
    }
    const char *strongest_cat = "Mixed";
    if (files_count > 0) {
        if (max_cat_idx == 0) strongest_cat = CATEGORY_STUDY;
        else if (max_cat_idx == 1) strongest_cat = CATEGORY_IMAGES;
        else if (max_cat_idx == 2) strongest_cat = CATEGORY_DOCUMENTS;
        else if (max_cat_idx == 3) strongest_cat = CATEGORY_VIDEOS;
        else strongest_cat = CATEGORY_OTHERS;
    }
    
    char summary_buf[2048];
    snprintf(summary_buf, sizeof(summary_buf), 
        "s0ucipher scanned %d file(s) from %d source(s). The strongest pattern is %s, and the planned output is %s.", 
        files_count, valid_sources_count, strongest_cat, dest_root);
    res->assistant.summary = strdup(summary_buf);
    
    // recommended actions
    char *actions[10];
    int acts_count = 0;
    if (res->study_files_count > 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Create a Study Hub with %d academic file(s).", res->study_files_count);
        actions[acts_count++] = strdup(buf);
    }
    if (res->important_files_count > 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Highlight %d exam/final file(s) as high priority.", res->important_files_count);
        actions[acts_count++] = strdup(buf);
    }
    if (res->duplicates_count > 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Review %d duplicate name group(s) before deleting anything.", res->duplicates_count);
        actions[acts_count++] = strdup(buf);
    }
    // Count cleanup old files
    int old_count = 0;
    for (int i = 0; i < files_count; i++) {
        int is_old = 0;
        for (int k = 0; k < res->before[i]->smart_tags_count; k++) {
            if (strcmp(res->before[i]->smart_tags[k], "cleanup-candidate") == 0) is_old = 1;
        }
        if (is_old) old_count++;
    }
    if (old_count > 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Archive or review %d old file(s).", old_count);
        actions[acts_count++] = strdup(buf);
    }
    if (acts_count == 0) {
        actions[acts_count++] = strdup("No urgent cleanup found. Organize by category and keep preview mode for safety.");
    }
    res->assistant.recommended_actions = malloc(sizeof(char*) * acts_count);
    for (int i = 0; i < acts_count; i++) res->assistant.recommended_actions[i] = actions[i];
    res->assistant.recommended_actions_count = acts_count;
    
    // study plan
    res->assistant.study_plan_count = 3;
    res->assistant.study_plan = malloc(sizeof(char*) * 3);
    res->assistant.study_plan[0] = strdup("Keep notes and assignments inside Study Hub.");
    res->assistant.study_plan[1] = strdup("Open high-priority files first before exams.");
    res->assistant.study_plan[2] = strdup("Use duplicate suggestions only for review, never automatic deletion.");
    
    if (apply_changes) {
        res->assistant.safety_note = strdup("Move mode is active. Files were sent to organized folders.");
    } else {
        res->assistant.safety_note = strdup("Preview mode is active, so files are not moved yet.");
    }
    
    // automation ideas
    res->assistant.automation_ideas_count = 4;
    res->assistant.automation_ideas = malloc(sizeof(char*) * 4);
    res->assistant.automation_ideas[0] = strdup("Suggest cleaner file names.");
    res->assistant.automation_ideas[1] = strdup("Detect exam files and important study material.");
    res->assistant.automation_ideas[2] = strdup("Separate media, documents, and uncategorized files.");
    res->assistant.automation_ideas[3] = strdup("Warn before moving duplicates or old files.");
    
    return res;
}

// In-memory organizer for web-mode
OrganizeResult *organize_metadata(json_value *files_metadata, const char *sort_by) {
    OrganizeResult *res = calloc(1, sizeof(OrganizeResult));
    
    int files_count = files_metadata ? files_metadata->u.array.count : 0;
    
    // Count duplicates by filename case-insensitively
    int *dup_counts = calloc(files_count + 1, sizeof(int));
    for (int i = 0; i < files_count; i++) {
        json_value *meta_i = files_metadata->u.array.items[i];
        const char *name_i = json_as_string(json_obj_get(meta_i, "name"));
        if (!name_i) name_i = "";
        
        int match_count = 0;
        for (int j = 0; j < files_count; j++) {
            json_value *meta_j = files_metadata->u.array.items[j];
            const char *name_j = json_as_string(json_obj_get(meta_j, "name"));
            if (!name_j) name_j = "";
            
            if (strcasecmp(name_i, name_j) == 0) {
                match_count++;
            }
        }
        dup_counts[i] = match_count;
    }
    
    // Build records
    res->before_count = files_count;
    res->before = malloc(sizeof(FileRecord*) * (files_count + 1));
    for (int i = 0; i < files_count; i++) {
        json_value *meta = files_metadata->u.array.items[i];
        
        const char *name = json_as_string(json_obj_get(meta, "name"));
        if (!name) name = "";
        
        const char *path = json_as_string(json_obj_get(meta, "path"));
        if (!path) path = name;
        
        long long size = (long long)json_as_number(json_obj_get(meta, "size_bytes"));
        if (size == 0) {
            size = (long long)json_as_number(json_obj_get(meta, "size"));
        }
        
        double last_mod = json_as_number(json_obj_get(meta, "lastModified"));
        time_t mtime = 0;
        if (last_mod != 0.0) {
            mtime = (time_t)(last_mod / 1000.0);
        } else {
            mtime = time(NULL);
        }
        
        res->before[i] = build_record(path, dup_counts[i], mtime, size);
        
        // In web metadata mode, the name key must match exactly what was sent
        free(res->before[i]->name);
        res->before[i]->name = strdup(name);
    }
    free(dup_counts);
    
    // Sort records using custom mergesort
    if (files_count > 0) {
        merge_sort(res->before, 0, files_count - 1, sort_by);
    }
    
    // Group duplicates
    res->duplicates = NULL;
    res->duplicates_count = 0;
    
    int *grouped = calloc(files_count + 1, sizeof(int));
    for (int i = 0; i < files_count; i++) {
        if (grouped[i]) continue;
        
        int count_copies = 0;
        for (int j = i; j < files_count; j++) {
            if (strcasecmp(res->before[i]->name, res->before[j]->name) == 0) {
                count_copies++;
            }
        }
        
        if (count_copies > 1) {
            DuplicateGroup *dg = malloc(sizeof(DuplicateGroup));
            dg->name = strdup(res->before[i]->name);
            dg->count = count_copies;
            dg->files = malloc(sizeof(FileRecord*) * count_copies);
            
            int idx = 0;
            for (int j = i; j < files_count; j++) {
                if (strcasecmp(res->before[i]->name, res->before[j]->name) == 0) {
                    dg->files[idx++] = res->before[j];
                    grouped[j] = 1;
                }
            }
            
            res->duplicates = realloc(res->duplicates, sizeof(DuplicateGroup*) * (res->duplicates_count + 1));
            res->duplicates[res->duplicates_count++] = dg;
        }
    }
    free(grouped);
    
    // Study files, important files, cleanup suggestions
    res->study_files = NULL;
    res->study_files_count = 0;
    
    res->important_files = NULL;
    res->important_files_count = 0;
    
    res->cleanup_suggestions = NULL;
    res->cleanup_suggestions_count = 0;
    
    res->moved_files = NULL;
    res->moved_files_count = 0;
    
    for (int i = 0; i < files_count; i++) {
        FileRecord *rec = res->before[i];
        
        if (strcmp(rec->category, CATEGORY_STUDY) == 0) {
            res->study_files = realloc(res->study_files, sizeof(FileRecord*) * (res->study_files_count + 1));
            res->study_files[res->study_files_count++] = rec;
        }
        
        if (strcmp(rec->priority, "High") == 0) {
            res->important_files = realloc(res->important_files, sizeof(FileRecord*) * (res->important_files_count + 1));
            res->important_files[res->important_files_count++] = rec;
        }
        
        int has_dup = 0;
        int has_old = 0;
        for (int k = 0; k < rec->smart_tags_count; k++) {
            if (strcmp(rec->smart_tags[k], "duplicate-candidate") == 0) has_dup = 1;
            if (strcmp(rec->smart_tags[k], "cleanup-candidate") == 0) has_old = 1;
        }
        
        if (has_dup) {
            CleanupSuggestion *cs = malloc(sizeof(CleanupSuggestion));
            cs->file = strdup(rec->name);
            cs->source = strdup(rec->source);
            cs->reason = strdup("Possible duplicate name across selected sources. Review before deleting.");
            
            res->cleanup_suggestions = realloc(res->cleanup_suggestions, sizeof(CleanupSuggestion*) * (res->cleanup_suggestions_count + 1));
            res->cleanup_suggestions[res->cleanup_suggestions_count++] = cs;
        }
        
        if (has_old) {
            CleanupSuggestion *cs = malloc(sizeof(CleanupSuggestion));
            cs->file = strdup(rec->name);
            cs->source = strdup(rec->source);
            cs->reason = strdup("Old file. Consider archiving or cleaning it up.");
            
            res->cleanup_suggestions = realloc(res->cleanup_suggestions, sizeof(CleanupSuggestion*) * (res->cleanup_suggestions_count + 1));
            res->cleanup_suggestions[res->cleanup_suggestions_count++] = cs;
        }
    }
    
    // Destination is fixed string in web mode
    res->path = strdup("NeuroSort Organized");
    res->destination_path = strdup("NeuroSort Organized");
    res->sort_by = strdup(sort_by);
    res->applied_changes = 0;
    res->sources_count = 0;
    res->sources = NULL;
    
    // Count unique categories
    int cats_present[5] = {0};
    for (int i = 0; i < files_count; i++) {
        if (strcmp(res->before[i]->category, CATEGORY_STUDY) == 0) cats_present[0] = 1;
        else if (strcmp(res->before[i]->category, CATEGORY_IMAGES) == 0) cats_present[1] = 1;
        else if (strcmp(res->before[i]->category, CATEGORY_DOCUMENTS) == 0) cats_present[2] = 1;
        else if (strcmp(res->before[i]->category, CATEGORY_VIDEOS) == 0) cats_present[3] = 1;
        else cats_present[4] = 1;
    }
    int cats_count = 0;
    for (int i = 0; i < 5; i++) cats_count += cats_present[i];
    
    res->dashboard.total_files = files_count;
    res->dashboard.source_count = 0;
    res->dashboard.total_categories = cats_count;
    res->dashboard.important_files = res->important_files_count;
    res->dashboard.study_files = res->study_files_count;
    res->dashboard.duplicate_groups = res->duplicates_count;
    res->dashboard.cleanup_items = res->cleanup_suggestions_count;
    
    // Assistant
    res->assistant.name = strdup("s0ucipher");
    
    int max_cat_idx = 4;
    int cat_counts[5] = {0};
    for (int i = 0; i < files_count; i++) {
        if (strcmp(res->before[i]->category, CATEGORY_STUDY) == 0) cat_counts[0]++;
        else if (strcmp(res->before[i]->category, CATEGORY_IMAGES) == 0) cat_counts[1]++;
        else if (strcmp(res->before[i]->category, CATEGORY_DOCUMENTS) == 0) cat_counts[2]++;
        else if (strcmp(res->before[i]->category, CATEGORY_VIDEOS) == 0) cat_counts[3]++;
        else cat_counts[4]++;
    }
    int max_val = -1;
    for (int i = 0; i < 5; i++) {
        if (cat_counts[i] > max_val) {
            max_val = cat_counts[i];
            max_cat_idx = i;
        }
    }
    const char *strongest_cat = "Mixed";
    if (files_count > 0) {
        if (max_cat_idx == 0) strongest_cat = CATEGORY_STUDY;
        else if (max_cat_idx == 1) strongest_cat = CATEGORY_IMAGES;
        else if (max_cat_idx == 2) strongest_cat = CATEGORY_DOCUMENTS;
        else if (max_cat_idx == 3) strongest_cat = CATEGORY_VIDEOS;
        else strongest_cat = CATEGORY_OTHERS;
    }
    
    char summary_buf[2048];
    snprintf(summary_buf, sizeof(summary_buf), 
        "s0ucipher scanned %d file(s) from %d source(s). The strongest pattern is %s, and the planned output is NeuroSort Organized.", 
        files_count, 0, strongest_cat);
    res->assistant.summary = strdup(summary_buf);
    
    char *actions[10];
    int acts_count = 0;
    if (res->study_files_count > 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Create a Study Hub with %d academic file(s).", res->study_files_count);
        actions[acts_count++] = strdup(buf);
    }
    if (res->important_files_count > 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Highlight %d exam/final file(s) as high priority.", res->important_files_count);
        actions[acts_count++] = strdup(buf);
    }
    if (res->duplicates_count > 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Review %d duplicate name group(s) before deleting anything.", res->duplicates_count);
        actions[acts_count++] = strdup(buf);
    }
    int old_count = 0;
    for (int i = 0; i < files_count; i++) {
        int is_old = 0;
        for (int k = 0; k < res->before[i]->smart_tags_count; k++) {
            if (strcmp(res->before[i]->smart_tags[k], "cleanup-candidate") == 0) is_old = 1;
        }
        if (is_old) old_count++;
    }
    if (old_count > 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Archive or review %d old file(s).", old_count);
        actions[acts_count++] = strdup(buf);
    }
    if (acts_count == 0) {
        actions[acts_count++] = strdup("No urgent cleanup found. Organize by category and keep preview mode for safety.");
    }
    res->assistant.recommended_actions = malloc(sizeof(char*) * acts_count);
    for (int i = 0; i < acts_count; i++) res->assistant.recommended_actions[i] = actions[i];
    res->assistant.recommended_actions_count = acts_count;
    
    res->assistant.study_plan_count = 3;
    res->assistant.study_plan = malloc(sizeof(char*) * 3);
    res->assistant.study_plan[0] = strdup("Keep notes and assignments inside Study Hub.");
    res->assistant.study_plan[1] = strdup("Open high-priority files first before exams.");
    res->assistant.study_plan[2] = strdup("Use duplicate suggestions only for review, never automatic deletion.");
    
    res->assistant.safety_note = strdup("Preview mode is active, so files are not moved yet.");
    
    res->assistant.automation_ideas_count = 4;
    res->assistant.automation_ideas = malloc(sizeof(char*) * 4);
    res->assistant.automation_ideas[0] = strdup("Suggest cleaner file names.");
    res->assistant.automation_ideas[1] = strdup("Detect exam files and important study material.");
    res->assistant.automation_ideas[2] = strdup("Separate media, documents, and uncategorized files.");
    res->assistant.automation_ideas[3] = strdup("Warn before moving duplicates or old files.");
    
    return res;
}

void free_organize_result(OrganizeResult *res) {
    if (!res) return;
    free(res->path);
    if (res->sources) {
        for (int i = 0; i < res->sources_count; i++) free(res->sources[i]);
        free(res->sources);
    }
    free(res->destination_path);
    free(res->sort_by);
    
    if (res->before) {
        for (int i = 0; i < res->before_count; i++) free_record(res->before[i]);
        free(res->before);
    }
    
    if (res->duplicates) {
        for (int i = 0; i < res->duplicates_count; i++) {
            free(res->duplicates[i]->name);
            free(res->duplicates[i]->files); // records themselves freed in res->before
            free(res->duplicates[i]);
        }
        free(res->duplicates);
    }
    
    free(res->study_files); // records freed in res->before
    free(res->important_files); // records freed in res->before
    
    if (res->cleanup_suggestions) {
        for (int i = 0; i < res->cleanup_suggestions_count; i++) {
            free(res->cleanup_suggestions[i]->file);
            free(res->cleanup_suggestions[i]->source);
            free(res->cleanup_suggestions[i]->reason);
            free(res->cleanup_suggestions[i]);
        }
        free(res->cleanup_suggestions);
    }
    
    if (res->moved_files) {
        for (int i = 0; i < res->moved_files_count; i++) {
            free(res->moved_files[i]->from);
            free(res->moved_files[i]->to);
            free(res->moved_files[i]);
        }
        free(res->moved_files);
    }
    
    // Assistant
    free(res->assistant.name);
    free(res->assistant.summary);
    if (res->assistant.recommended_actions) {
        for (int i = 0; i < res->assistant.recommended_actions_count; i++) {
            free(res->assistant.recommended_actions[i]);
        }
        free(res->assistant.recommended_actions);
    }
    if (res->assistant.study_plan) {
        for (int i = 0; i < res->assistant.study_plan_count; i++) {
            free(res->assistant.study_plan[i]);
        }
        free(res->assistant.study_plan);
    }
    free(res->assistant.safety_note);
    if (res->assistant.automation_ideas) {
        for (int i = 0; i < res->assistant.automation_ideas_count; i++) {
            free(res->assistant.automation_ideas[i]);
        }
        free(res->assistant.automation_ideas);
    }
    
    free(res);
}

// Serialization
static void serialize_record_sb(StringBuilder *sb, const FileRecord *rec) {
    sb_append(sb, "{");
    
    sb_append(sb, "\"name\":\"");
    char *escaped_name = escape_json_string(rec->name);
    sb_append(sb, escaped_name);
    free(escaped_name);
    sb_append(sb, "\",");
    
    sb_append(sb, "\"source\":\"");
    char *escaped_src = escape_json_string(rec->source);
    sb_append(sb, escaped_src);
    free(escaped_src);
    sb_append(sb, "\",");
    
    sb_append(sb, "\"type\":\"");
    sb_append(sb, rec->type);
    sb_append(sb, "\",");
    
    sb_append(sb, "\"category\":\"");
    sb_append(sb, rec->category);
    sb_append(sb, "\",");
    
    sb_append(sb, "\"priority\":\"");
    sb_append(sb, rec->priority);
    sb_append(sb, "\",");
    
    sb_append(sb, "\"folder\":\"");
    sb_append(sb, rec->folder);
    sb_append(sb, "\",");
    
    char num_buf[64];
    snprintf(num_buf, sizeof(num_buf), "%lld", rec->size_bytes);
    sb_append(sb, "\"size_bytes\":");
    sb_append(sb, num_buf);
    sb_append(sb, ",");
    
    sb_append(sb, "\"modified_at\":\"");
    sb_append(sb, rec->modified_at);
    sb_append(sb, "\",");
    
    sb_append(sb, "\"smart_tags\":[");
    for (int i = 0; i < rec->smart_tags_count; i++) {
        if (i > 0) sb_append(sb, ",");
        sb_append(sb, "\"");
        sb_append(sb, rec->smart_tags[i]);
        sb_append(sb, "\"");
    }
    sb_append(sb, "],");
    
    sb_append(sb, "\"ai_reason\":\"");
    char *escaped_reason = escape_json_string(rec->ai_reason);
    sb_append(sb, escaped_reason);
    free(escaped_reason);
    sb_append(sb, "\",");
    
    snprintf(num_buf, sizeof(num_buf), "%.2f", rec->confidence);
    sb_append(sb, "\"confidence\":");
    sb_append(sb, num_buf);
    sb_append(sb, ",");
    
    sb_append(sb, "\"suggested_name\":\"");
    char *escaped_sug = escape_json_string(rec->suggested_name);
    sb_append(sb, escaped_sug);
    free(escaped_sug);
    sb_append(sb, "\"");
    
    sb_append(sb, "}");
}

char *serialize_organize_result(const OrganizeResult *res) {
    StringBuilder sb;
    sb_init(&sb);
    
    sb_append(&sb, "{");
    
    sb_append(&sb, "\"path\":\"");
    char *escaped_path = escape_json_string(res->path);
    sb_append(&sb, escaped_path);
    free(escaped_path);
    sb_append(&sb, "\",");
    
    sb_append(&sb, "\"destination_path\":\"");
    char *escaped_dest = escape_json_string(res->destination_path);
    sb_append(&sb, escaped_dest);
    free(escaped_dest);
    sb_append(&sb, "\",");
    
    sb_append(&sb, "\"sort_by\":\"");
    sb_append(&sb, res->sort_by);
    sb_append(&sb, "\",");
    
    sb_append(&sb, "\"applied_changes\":");
    sb_append(&sb, res->applied_changes ? "true" : "false");
    sb_append(&sb, ",");
    
    // sources array
    sb_append(&sb, "\"sources\":[");
    for (int i = 0; i < res->sources_count; i++) {
        if (i > 0) sb_append(&sb, ",");
        sb_append(&sb, "\"");
        char *escaped_src = escape_json_string(res->sources[i]);
        sb_append(&sb, escaped_src);
        free(escaped_src);
        sb_append(&sb, "\"");
    }
    sb_append(&sb, "],");
    
    // before array
    sb_append(&sb, "\"before\":[");
    for (int i = 0; i < res->before_count; i++) {
        if (i > 0) sb_append(&sb, ",");
        serialize_record_sb(&sb, res->before[i]);
    }
    sb_append(&sb, "],");
    
    // categories dictionary
    sb_append(&sb, "\"categories\":{");
    const char *cats[] = {CATEGORY_STUDY, CATEGORY_IMAGES, CATEGORY_DOCUMENTS, CATEGORY_VIDEOS, CATEGORY_OTHERS};
    int first_cat = 1;
    for (int c = 0; c < 5; c++) {
        int cat_count = 0;
        for (int i = 0; i < res->before_count; i++) {
            if (strcmp(res->before[i]->category, cats[c]) == 0) cat_count++;
        }
        if (cat_count == 0) continue;
        if (!first_cat) sb_append(&sb, ",");
        first_cat = 0;
        sb_append(&sb, "\"");
        sb_append(&sb, cats[c]);
        sb_append(&sb, "\":[");
        int first_rec = 1;
        for (int i = 0; i < res->before_count; i++) {
            if (strcmp(res->before[i]->category, cats[c]) == 0) {
                if (!first_rec) sb_append(&sb, ",");
                first_rec = 0;
                serialize_record_sb(&sb, res->before[i]);
            }
        }
        sb_append(&sb, "]");
    }
    sb_append(&sb, "},");
    
    // after_structure dictionary
    sb_append(&sb, "\"after_structure\":{");
    const char *folders[] = {FOLDER_STUDY, FOLDER_IMAGES, FOLDER_DOCUMENTS, FOLDER_VIDEOS, FOLDER_OTHERS};
    int first_fld = 1;
    for (int f = 0; f < 5; f++) {
        int fld_count = 0;
        for (int i = 0; i < res->before_count; i++) {
            if (strcmp(res->before[i]->folder, folders[f]) == 0) fld_count++;
        }
        if (fld_count == 0) continue;
        if (!first_fld) sb_append(&sb, ",");
        first_fld = 0;
        sb_append(&sb, "\"");
        sb_append(&sb, folders[f]);
        sb_append(&sb, "\":[");
        int first_rec = 1;
        for (int i = 0; i < res->before_count; i++) {
            if (strcmp(res->before[i]->folder, folders[f]) == 0) {
                if (!first_rec) sb_append(&sb, ",");
                first_rec = 0;
                serialize_record_sb(&sb, res->before[i]);
            }
        }
        sb_append(&sb, "]");
    }
    sb_append(&sb, "},");
    
    // duplicates array
    sb_append(&sb, "\"duplicates\":[");
    for (int i = 0; i < res->duplicates_count; i++) {
        if (i > 0) sb_append(&sb, ",");
        sb_append(&sb, "{\"name\":\"");
        char *escaped_name = escape_json_string(res->duplicates[i]->name);
        sb_append(&sb, escaped_name);
        free(escaped_name);
        sb_append(&sb, "\",\"count\":");
        char num_buf[32];
        snprintf(num_buf, sizeof(num_buf), "%d", res->duplicates[i]->count);
        sb_append(&sb, num_buf);
        sb_append(&sb, ",\"files\":[");
        for (int j = 0; j < res->duplicates[i]->count; j++) {
            if (j > 0) sb_append(&sb, ",");
            serialize_record_sb(&sb, res->duplicates[i]->files[j]);
        }
        sb_append(&sb, "]}");
    }
    sb_append(&sb, "],");
    
    // study_files array
    sb_append(&sb, "\"study_files\":[");
    for (int i = 0; i < res->study_files_count; i++) {
        if (i > 0) sb_append(&sb, ",");
        serialize_record_sb(&sb, res->study_files[i]);
    }
    sb_append(&sb, "],");
    
    // important_files array
    sb_append(&sb, "\"important_files\":[");
    for (int i = 0; i < res->important_files_count; i++) {
        if (i > 0) sb_append(&sb, ",");
        serialize_record_sb(&sb, res->important_files[i]);
    }
    sb_append(&sb, "],");
    
    // priorities dictionary (High, Medium, Low)
    sb_append(&sb, "\"priorities\":{");
    const char *prios[] = {"High", "Medium", "Low"};
    for (int p = 0; p < 3; p++) {
        if (p > 0) sb_append(&sb, ",");
        sb_append(&sb, "\"");
        sb_append(&sb, prios[p]);
        sb_append(&sb, "\":[");
        int first_rec = 1;
        for (int i = 0; i < res->before_count; i++) {
            if (strcmp(res->before[i]->priority, prios[p]) == 0) {
                if (!first_rec) sb_append(&sb, ",");
                first_rec = 0;
                serialize_record_sb(&sb, res->before[i]);
            }
        }
        sb_append(&sb, "]");
    }
    sb_append(&sb, "},");
    
    // cleanup_suggestions array
    sb_append(&sb, "\"cleanup_suggestions\":[");
    for (int i = 0; i < res->cleanup_suggestions_count; i++) {
        if (i > 0) sb_append(&sb, ",");
        sb_append(&sb, "{\"file\":\"");
        char *escaped_file = escape_json_string(res->cleanup_suggestions[i]->file);
        sb_append(&sb, escaped_file);
        free(escaped_file);
        sb_append(&sb, "\",\"source\":\"");
        char *escaped_src = escape_json_string(res->cleanup_suggestions[i]->source);
        sb_append(&sb, escaped_src);
        free(escaped_src);
        sb_append(&sb, "\",\"reason\":\"");
        char *escaped_reason = escape_json_string(res->cleanup_suggestions[i]->reason);
        sb_append(&sb, escaped_reason);
        free(escaped_reason);
        sb_append(&sb, "\"}");
    }
    sb_append(&sb, "],");
    
    // moved_files array
    sb_append(&sb, "\"moved_files\":[");
    for (int i = 0; i < res->moved_files_count; i++) {
        if (i > 0) sb_append(&sb, ",");
        sb_append(&sb, "{\"from\":\"");
        char *escaped_from = escape_json_string(res->moved_files[i]->from);
        sb_append(&sb, escaped_from);
        free(escaped_from);
        sb_append(&sb, "\",\"to\":\"");
        char *escaped_to = escape_json_string(res->moved_files[i]->to);
        sb_append(&sb, escaped_to);
        free(escaped_to);
        sb_append(&sb, "\"}");
    }
    sb_append(&sb, "],");
    
    // dashboard object
    char db_buf[1024];
    snprintf(db_buf, sizeof(db_buf), 
        "\"dashboard\":{\"total_files\":%d,\"source_count\":%d,\"total_categories\":%d,\"important_files\":%d,\"study_files\":%d,\"duplicate_groups\":%d,\"cleanup_items\":%d},", 
        res->dashboard.total_files, res->dashboard.source_count, res->dashboard.total_categories, 
        res->dashboard.important_files, res->dashboard.study_files, res->dashboard.duplicate_groups, res->dashboard.cleanup_items);
    sb_append(&sb, db_buf);
    
    // assistant object
    sb_append(&sb, "\"assistant\":{");
    
    sb_append(&sb, "\"name\":\"");
    sb_append(&sb, res->assistant.name);
    sb_append(&sb, "\",");
    
    sb_append(&sb, "\"summary\":\"");
    char *escaped_sum = escape_json_string(res->assistant.summary);
    sb_append(&sb, escaped_sum);
    free(escaped_sum);
    sb_append(&sb, "\",");
    
    sb_append(&sb, "\"safety_note\":\"");
    char *escaped_safety = escape_json_string(res->assistant.safety_note);
    sb_append(&sb, escaped_safety);
    free(escaped_safety);
    sb_append(&sb, "\",");
    
    // recommended_actions array
    sb_append(&sb, "\"recommended_actions\":[");
    for (int i = 0; i < res->assistant.recommended_actions_count; i++) {
        if (i > 0) sb_append(&sb, ",");
        sb_append(&sb, "\"");
        char *escaped_act = escape_json_string(res->assistant.recommended_actions[i]);
        sb_append(&sb, escaped_act);
        free(escaped_act);
        sb_append(&sb, "\"");
    }
    sb_append(&sb, "],");
    
    // study_plan array
    sb_append(&sb, "\"study_plan\":[");
    for (int i = 0; i < res->assistant.study_plan_count; i++) {
        if (i > 0) sb_append(&sb, ",");
        sb_append(&sb, "\"");
        char *escaped_sp = escape_json_string(res->assistant.study_plan[i]);
        sb_append(&sb, escaped_sp);
        free(escaped_sp);
        sb_append(&sb, "\"");
    }
    sb_append(&sb, "],");
    
    // automation_ideas array
    sb_append(&sb, "\"automation_ideas\":[");
    for (int i = 0; i < res->assistant.automation_ideas_count; i++) {
        if (i > 0) sb_append(&sb, ",");
        sb_append(&sb, "\"");
        char *escaped_idea = escape_json_string(res->assistant.automation_ideas[i]);
        sb_append(&sb, escaped_idea);
        free(escaped_idea);
        sb_append(&sb, "\"");
    }
    sb_append(&sb, "]");
    
    sb_append(&sb, "}"); // end assistant
    
    sb_append(&sb, "}"); // end result
    
    char *res_str = strdup(sb.buf);
    sb_free(&sb);
    return res_str;
}

// Save Organized Copy (Mode downloads / custom)
char *save_organized_copy(char **sources, int sources_count, const char *sort_by, const char *destination_path, const char *save_mode) {
    // 1. Run organize preview to get structures
    OrganizeResult *preview = organize_sources(sources, sources_count, sort_by, 0, destination_path);
    
    // 2. Determine target save root folder
    char target_root[PATH_MAX];
    if (strcmp(save_mode, "downloads") == 0) {
        // Find user Downloads folder (~/Downloads)
        const char *home = getenv("HOME");
        if (!home) home = ".";
        snprintf(target_root, sizeof(target_root), "%s/Downloads", home);
    } else {
        if (destination_path && strlen(destination_path) > 0) {
            strcpy(target_root, destination_path);
        } else {
            strcpy(target_root, ".");
        }
    }
    
    // 3. Make export root folder name with timestamp
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d_%H-%M-%S", tm_now);
    
    char export_path[PATH_MAX];
    snprintf(export_path, sizeof(export_path), "%s/NeuroSort Export %s", target_root, time_str);
    
    // 4. Copy each file
    StringBuilder sb_copied;
    StringBuilder sb_skipped;
    sb_init(&sb_copied);
    sb_init(&sb_skipped);
    
    int copied_count = 0;
    int skipped_count = 0;
    
    for (int i = 0; i < preview->before_count; i++) {
        FileRecord *rec = preview->before[i];
        
        struct stat st;
        if (stat(rec->source, &st) != 0) {
            if (skipped_count > 0) sb_append(&sb_skipped, ",");
            sb_append(&sb_skipped, "{\"file\":\"");
            char *escaped_file = escape_json_string(rec->name);
            sb_append(&sb_skipped, escaped_file);
            free(escaped_file);
            sb_append(&sb_skipped, "\",\"source\":\"");
            char *escaped_src = escape_json_string(rec->source);
            sb_append(&sb_skipped, escaped_src);
            free(escaped_src);
            sb_append(&sb_skipped, "\",\"reason\":\"Source file was not found.\"}");
            skipped_count++;
            continue;
        }
        
        char target_dir[PATH_MAX];
        char target_dest[PATH_MAX];
        get_safe_destination(export_path, rec->folder, rec->name, target_dir, target_dest, sizeof(target_dest));
        
        mkdir_p(target_dir);
        if (copy_file(rec->source, target_dest) == 0) {
            if (copied_count > 0) sb_append(&sb_copied, ",");
            sb_append(&sb_copied, "{\"from\":\"");
            char *escaped_from = escape_json_string(rec->source);
            sb_append(&sb_copied, escaped_from);
            free(escaped_from);
            sb_append(&sb_copied, "\",\"to\":\"");
            char *escaped_to = escape_json_string(target_dest);
            sb_append(&sb_copied, escaped_to);
            free(escaped_to);
            sb_append(&sb_copied, "\"}");
            copied_count++;
        } else {
            if (skipped_count > 0) sb_append(&sb_skipped, ",");
            sb_append(&sb_skipped, "{\"file\":\"");
            char *escaped_file = escape_json_string(rec->name);
            sb_append(&sb_skipped, escaped_file);
            free(escaped_file);
            sb_append(&sb_skipped, "\",\"source\":\"");
            char *escaped_src = escape_json_string(rec->source);
            sb_append(&sb_skipped, escaped_src);
            free(escaped_src);
            sb_append(&sb_skipped, "\",\"reason\":\"Failed to copy file.\"}");
            skipped_count++;
        }
    }
    
    char assistant_msg[2048];
    if (copied_count > 0) {
        snprintf(assistant_msg, sizeof(assistant_msg), "s0ucipher saved %d organized file(s) to %s.", copied_count, export_path);
    } else {
        strcpy(assistant_msg, "s0ucipher could not save files because no source files were available.");
    }
    
    // Construct final JSON response
    StringBuilder res_sb;
    sb_init(&res_sb);
    
    sb_append(&res_sb, "{\"export_path\":\"");
    char *escaped_exp = escape_json_string(export_path);
    sb_append(&res_sb, escaped_exp);
    free(escaped_exp);
    sb_append(&res_sb, "\",\"copied_count\":");
    char num_buf[32];
    snprintf(num_buf, sizeof(num_buf), "%d", copied_count);
    sb_append(&res_sb, num_buf);
    sb_append(&res_sb, ",\"skipped_count\":");
    snprintf(num_buf, sizeof(num_buf), "%d", skipped_count);
    sb_append(&res_sb, num_buf);
    sb_append(&res_sb, ",\"save_mode\":\"");
    sb_append(&res_sb, save_mode);
    sb_append(&res_sb, "\",\"assistant_message\":\"");
    char *escaped_msg = escape_json_string(assistant_msg);
    sb_append(&res_sb, escaped_msg);
    free(escaped_msg);
    sb_append(&res_sb, "\",\"copied_files\":[");
    sb_append(&res_sb, sb_copied.buf);
    sb_append(&res_sb, "],\"skipped_files\":[");
    sb_append(&res_sb, sb_skipped.buf);
    sb_append(&res_sb, "]}");
    
    char *res_str = strdup(res_sb.buf);
    
    sb_free(&res_sb);
    sb_free(&sb_copied);
    sb_free(&sb_skipped);
    free_organize_result(preview);
    
    return res_str;
}

// File copy
int copy_file(const char *src, const char *dest) {
    FILE *in = fopen(src, "rb");
    if (!in) return -1;
    FILE *out = fopen(dest, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }
    char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            return -1;
        }
    }
    fclose(in);
    fclose(out);
    return 0;
}

// File move (handles cross-device)
int move_file(const char *src, const char *dest) {
    if (rename(src, dest) == 0) {
        return 0;
    }
    // Cross-device rename fails, copy and delete
    if (copy_file(src, dest) == 0) {
        unlink(src);
        return 0;
    }
    return -1;
}

// Recursive directory creation
void mkdir_p(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0777);
            *p = '/';
        }
    }
    mkdir(tmp, 0777);
}

// JSON Escaping
char *escape_json_string(const char *str) {
    if (!str) return strdup("null");
    size_t len = 0;
    for (const char *p = str; *p; p++) {
        if (*p == '"' || *p == '\\' || *p == '\n' || *p == '\r' || *p == '\t') {
            len += 2;
        } else {
            len++;
        }
    }
    char *res = malloc(len + 1);
    char *dst = res;
    for (const char *p = str; *p; p++) {
        if (*p == '"') { *dst++ = '\\'; *dst++ = '"'; }
        else if (*p == '\\') { *dst++ = '\\'; *dst++ = '\\'; }
        else if (*p == '\n') { *dst++ = '\\'; *dst++ = 'n'; }
        else if (*p == '\r') { *dst++ = '\\'; *dst++ = 'r'; }
        else if (*p == '\t') { *dst++ = '\\'; *dst++ = 't'; }
        else { *dst++ = *p; }
    }
    *dst = '\0';
    return res;
}
