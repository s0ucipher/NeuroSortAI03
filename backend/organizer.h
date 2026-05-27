#ifndef ORGANIZER_H
#define ORGANIZER_H

#include "json.h"

typedef struct {
    char *name;
    char *source;
    char *type;
    char *category;
    char *priority;
    char *folder;
    long long size_bytes;
    char *modified_at;
    char **smart_tags;
    int smart_tags_count;
    char *ai_reason;
    double confidence;
    char *suggested_name;
} FileRecord;

typedef struct {
    char *name;
    int count;
    FileRecord **files;
} DuplicateGroup;

typedef struct {
    char *file;
    char *source;
    char *reason;
} CleanupSuggestion;

typedef struct {
    char *from;
    char *to;
} MovedFile;

typedef struct {
    int total_files;
    int source_count;
    int total_categories;
    int important_files;
    int study_files;
    int duplicate_groups;
    int cleanup_items;
} Dashboard;

typedef struct {
    char *name;
    char *summary;
    char **recommended_actions;
    int recommended_actions_count;
    char **study_plan;
    int study_plan_count;
    char *safety_note;
    char **automation_ideas;
    int automation_ideas_count;
} Assistant;

typedef struct {
    char *path;
    char **sources;
    int sources_count;
    char *destination_path;
    char *sort_by;
    int applied_changes;
    FileRecord **before;
    int before_count;
    DuplicateGroup **duplicates;
    int duplicates_count;
    FileRecord **study_files;
    int study_files_count;
    FileRecord **important_files;
    int important_files_count;
    CleanupSuggestion **cleanup_suggestions;
    int cleanup_suggestions_count;
    MovedFile **moved_files;
    int moved_files_count;
    Dashboard dashboard;
    Assistant assistant;
} OrganizeResult;

// Core functions
OrganizeResult *organize_sources(char **sources, int sources_count, const char *sort_by, int apply_changes, const char *destination_path);
OrganizeResult *organize_metadata(json_value *files_metadata, const char *sort_by);
void free_organize_result(OrganizeResult *res);
char *serialize_organize_result(const OrganizeResult *res);

// Save copying organized copies
char *save_organized_copy(char **sources, int sources_count, const char *sort_by, const char *destination_path, const char *save_mode);

// Helper file utilities
int copy_file(const char *src, const char *dest);
int move_file(const char *src, const char *dest);
void mkdir_p(const char *path);
char *escape_json_string(const char *str);

#endif /* ORGANIZER_H */
