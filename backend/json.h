#ifndef JSON_H
#define JSON_H

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type;

typedef struct json_value json_value;

typedef struct {
    char *key;
    json_value *val;
} json_object_entry;

struct json_value {
    json_type type;
    union {
        int boolean;
        double number;
        char *string;
        struct {
            json_value **items;
            int count;
        } array;
        struct {
            json_object_entry *entries;
            int count;
        } object;
    } u;
};

// Parse a JSON string. Returns NULL on syntax error.
json_value *json_parse(const char *str);

// Free all memory occupied by a JSON value.
void json_free(json_value *val);

// Helper functions for easy querying
json_value *json_obj_get(const json_value *obj, const char *key);
const char *json_as_string(const json_value *val);
double json_as_number(const json_value *val);
int json_as_bool(const json_value *val);
int json_is_null(const json_value *val);

#endif /* JSON_H */
