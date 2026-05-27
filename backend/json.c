#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void skip_whitespace(const char **p) {
    while (**p && (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r')) {
        (*p)++;
    }
}

static json_value *parse_value(const char **p);
static json_value *parse_string(const char **p);
static json_value *parse_number(const char **p);
static json_value *parse_object(const char **p);
static json_value *parse_array(const char **p);

static json_value *parse_literal(const char **p, const char *literal, json_type type, int val_bool) {
    size_t len = strlen(literal);
    if (strncmp(*p, literal, len) == 0) {
        *p += len;
        json_value *val = calloc(1, sizeof(json_value));
        val->type = type;
        if (type == JSON_BOOL) {
            val->u.boolean = val_bool;
        }
        return val;
    }
    return NULL;
}

static json_value *parse_string(const char **p) {
    if (**p != '"') return NULL;
    (*p)++; // skip '"'
    
    const char *start = *p;
    size_t len = 0;
    while (**p && **p != '"') {
        if (**p == '\\') {
            (*p)++;
            if (**p == '\0') return NULL;
        }
        len++;
        (*p)++;
    }
    if (**p != '"') return NULL;
    
    char *str = malloc(len + 1);
    *p = start;
    char *dst = str;
    while (**p && **p != '"') {
        if (**p == '\\') {
            (*p)++;
            switch (**p) {
                case '"':  *dst++ = '"'; break;
                case '\\': *dst++ = '\\'; break;
                case '/':  *dst++ = '/'; break;
                case 'b':  *dst++ = '\b'; break;
                case 'f':  *dst++ = '\f'; break;
                case 'n':  *dst++ = '\n'; break;
                case 'r':  *dst++ = '\r'; break;
                case 't':  *dst++ = '\t'; break;
                default:   *dst++ = **p; break;
            }
        } else {
            *dst++ = **p;
        }
        (*p)++;
    }
    *dst = '\0';
    (*p)++; // skip '"'
    
    json_value *val = calloc(1, sizeof(json_value));
    val->type = JSON_STRING;
    val->u.string = str;
    return val;
}

static json_value *parse_number(const char **p) {
    char *end;
    double num = strtod(*p, &end);
    if (end == *p) return NULL;
    *p = end;
    
    json_value *val = calloc(1, sizeof(json_value));
    val->type = JSON_NUMBER;
    val->u.number = num;
    return val;
}

static json_value *parse_array(const char **p) {
    if (**p != '[') return NULL;
    (*p)++; // skip '['
    skip_whitespace(p);
    
    json_value **items = NULL;
    int count = 0;
    
    if (**p == ']') {
        (*p)++;
        json_value *val = calloc(1, sizeof(json_value));
        val->type = JSON_ARRAY;
        val->u.array.items = NULL;
        val->u.array.count = 0;
        return val;
    }
    
    while (1) {
        json_value *item = parse_value(p);
        if (!item) {
            for (int i = 0; i < count; i++) json_free(items[i]);
            free(items);
            return NULL;
        }
        items = realloc(items, sizeof(json_value*) * (count + 1));
        items[count++] = item;
        
        skip_whitespace(p);
        if (**p == ']') {
            (*p)++;
            break;
        } else if (**p == ',') {
            (*p)++;
            skip_whitespace(p);
        } else {
            for (int i = 0; i < count; i++) json_free(items[i]);
            free(items);
            return NULL;
        }
    }
    
    json_value *val = calloc(1, sizeof(json_value));
    val->type = JSON_ARRAY;
    val->u.array.items = items;
    val->u.array.count = count;
    return val;
}

static json_value *parse_object(const char **p) {
    if (**p != '{') return NULL;
    (*p)++; // skip '{'
    skip_whitespace(p);
    
    json_object_entry *entries = NULL;
    int count = 0;
    
    if (**p == '}') {
        (*p)++;
        json_value *val = calloc(1, sizeof(json_value));
        val->type = JSON_OBJECT;
        val->u.object.entries = NULL;
        val->u.object.count = 0;
        return val;
    }
    
    while (1) {
        skip_whitespace(p);
        if (**p != '"') goto error;
        
        json_value *key_val = parse_string(p);
        if (!key_val) goto error;
        char *key = key_val->u.string;
        free(key_val);
        
        skip_whitespace(p);
        if (**p != ':') {
            free(key);
            goto error;
        }
        (*p)++; // skip ':'
        skip_whitespace(p);
        
        json_value *item = parse_value(p);
        if (!item) {
            free(key);
            goto error;
        }
        
        entries = realloc(entries, sizeof(json_object_entry) * (count + 1));
        entries[count].key = key;
        entries[count].val = item;
        count++;
        
        skip_whitespace(p);
        if (**p == '}') {
            (*p)++;
            break;
        } else if (**p == ',') {
            (*p)++;
            skip_whitespace(p);
        } else {
            goto error;
        }
    }
    
    json_value *val = calloc(1, sizeof(json_value));
    val->type = JSON_OBJECT;
    val->u.object.entries = entries;
    val->u.object.count = count;
    return val;
    
error:
    for (int i = 0; i < count; i++) {
        free(entries[i].key);
        json_free(entries[i].val);
    }
    free(entries);
    return NULL;
}

static json_value *parse_value(const char **p) {
    skip_whitespace(p);
    if (**p == '\0') return NULL;
    if (**p == '{') return parse_object(p);
    if (**p == '[') return parse_array(p);
    if (**p == '"') return parse_string(p);
    if (**p == 't') return parse_literal(p, "true", JSON_BOOL, 1);
    if (**p == 'f') return parse_literal(p, "false", JSON_BOOL, 0);
    if (**p == 'n') return parse_literal(p, "null", JSON_NULL, 0);
    return parse_number(p);
}

json_value *json_parse(const char *str) {
    const char *p = str;
    json_value *val = parse_value(&p);
    if (val) {
        skip_whitespace(&p);
        if (*p != '\0') {
            json_free(val);
            return NULL;
        }
    }
    return val;
}

void json_free(json_value *val) {
    if (!val) return;
    switch (val->type) {
        case JSON_STRING:
            free(val->u.string);
            break;
        case JSON_ARRAY:
            for (int i = 0; i < val->u.array.count; i++) {
                json_free(val->u.array.items[i]);
            }
            free(val->u.array.items);
            break;
        case JSON_OBJECT:
            for (int i = 0; i < val->u.object.count; i++) {
                free(val->u.object.entries[i].key);
                json_free(val->u.object.entries[i].val);
            }
            free(val->u.object.entries);
            break;
        default:
            break;
    }
    free(val);
}

json_value *json_obj_get(const json_value *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJECT) return NULL;
    for (int i = 0; i < obj->u.object.count; i++) {
        if (strcmp(obj->u.object.entries[i].key, key) == 0) {
            return obj->u.object.entries[i].val;
        }
    }
    return NULL;
}

const char *json_as_string(const json_value *val) {
    if (val && val->type == JSON_STRING) {
        return val->u.string;
    }
    return NULL;
}

double json_as_number(const json_value *val) {
    if (val && val->type == JSON_NUMBER) {
        return val->u.number;
    }
    return 0.0;
}

int json_as_bool(const json_value *val) {
    if (val && val->type == JSON_BOOL) {
        return val->u.boolean;
    }
    return 0;
}

int json_is_null(const json_value *val) {
    return (!val || val->type == JSON_NULL);
}
