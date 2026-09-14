#ifndef LSF_TABLE_H
#define LSF_TABLE_H

typedef struct {
    char **headers;
    int nheaders;
    char ***rows;
    int *rowlens;
    int nrows;
} Table;

Table parse_table(char **lines, int nlines);
void table_free(Table *t);

int table_col(Table *t, const char *name);
const char *table_get(Table *t, int r, int c);

#endif