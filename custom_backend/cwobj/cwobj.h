/*
    Copyright (c) 2018, Julien Verneuil
    All rights reserved.
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the organization nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL Julien Verneuil BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CWOBJ_H_
#define CWOBJ_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/** @brief indices type*/
#define CWOBJ_INDICE_TYPE unsigned int
/** @brief max line length in .obj and .mtl file*/
#define CWOBJ_MAX_LINE_LENGTH 512

/*
    A set of tools related to string processing
*/

//! case insensitive compare
/*!
    \param str1first string
    \param str2 second string
    \param length max length
    \return 0 if equal, 1 otherwise
*/
int cwobj_strncmpi(const char *str1, const char *str2, int length);

//! split a string given a delimiter (result must be freed with cwobj_freeSplitResult)
/*!
    \param str string to split
    \param delimiter delimiter
    \param n a pointer holding the length of the resulting array
    \return array of strings
*/
char **cwobj_splitString(const char *str, char delimiter, long *n);

//! free a cwobj_splitString result
/*!
    \param split_result array of strings
    \param n length of the given array
*/
void cwobj_freeSplitResult(char **split_result, long n);

//! transform a string to lowercase (in place)
/*!
    \param string pointer to a string
*/
void cwobj_toLowercase(char *string);

//! get a file path (without filename, separated by '/' only)
/*!
    \param string filepath
    \return path
*/
char *cwobj_getPath(const char *string);

//! parse an arbitrary value (interger or real)
/*!
    \param sp value pointer
    \return parsed value as bytes, NULL if not a value
*/
char *cwobj_parseValue(char **sp);

//! parse a float
/*!
    \param sp value pointer
    \return float value
*/
float cwobj_parseFloat(char **sp);

//! parse an integer
/*!
    \param sp value pointer
    \return integer value
*/
int cwobj_parseInt(char **sp);

inline int cwobj_strncmpi(const char *str1, const char *str2, int length) {
    int k;
    for (k = 0; k < length; k += 1) {
        if ((str1[k] | 32) != (str2[k] | 32))
            break;
    }

    if (k != length)
        return 1;
    return 0;
}

inline void cwobj_toLowercase(char *string) {
    if (!string) return;

    char *p = string;
    while (*p != '\0') {
        if (*p >= 'A' && *p <= 'Z') {
            *p += 32;
        }

        p++;
    }
}

inline void cwobj_freeSplitResult(char **split_result, long n) {
    while(n--) {
        free(split_result[n]);
    };
    free(split_result);
}

inline char **cwobj_splitString(const char *str, char delimiter, long *n) {
    if (!str) return NULL;

    char **split_result = 0;
    long strings_count = 0;

    const char *begin = str;
    while (*str++ != '\0') {
        size_t new_str_length = (str-begin);
        //if (*(str-1) == delimiter) {
           // begin = str; // make it ignore a sequence of delimiters
           // continue;
        //}

        if (*str == delimiter || *str == '\0') {
            char *new_str = NULL;
            if (new_str_length > 0) {
                new_str = (char *)malloc(sizeof(char) * (new_str_length+1));
                if (!new_str) continue;

                strncpy(new_str, begin, new_str_length);
                new_str[new_str_length] = '\0';
            }

            begin = str+1;

            char **split_result_tmp = (char **)realloc(split_result,
                                        sizeof(char **) * (strings_count+1));
            if (!split_result_tmp) {
                free(new_str);
                continue;
            }

            split_result = split_result_tmp;
            split_result[strings_count] = new_str;

            strings_count++;
        }
    }

    *n = strings_count;

    return split_result;
}

inline char *cwobj_getPath(const char *string) {
    if (!string)  return NULL;
    if (!*string) return NULL;

    size_t length = strlen(string);

    const char *end = string+length;
    int new_length = 0;
    while (*--end != '/' && (new_length = end-string) > 0);

    if (!new_length) {
        return NULL;
    }

    char *path = (char *)malloc(sizeof(char) * (new_length+1));
    strncpy(path, string, new_length);
    path[new_length] = '\0';

    return path;
}

inline char *cwobj_parseValue(char **sp) {
    if (!sp) {
        return NULL;
    }

	int j = 0;
	char *value_s = (char *)malloc(sizeof(char) * 32);

	if (!value_s) {
        fprintf(stderr, "cwobj_parseValue: malloc failed\n");

        return NULL;
    }

    memset(value_s, 0, sizeof(char) * 32);

    int ignore = 0;
	while ((*(*sp)) != '\0') {
        if ((*(*sp) >= 48 && *(*sp) < 58 && j < 32 && ignore == 0) ||
			*(*sp) == '.' || *(*sp) == '-') {
            value_s[j++] = (*(*sp));
        } else {
            if (j) {
                ignore = 1;

                if ((*(*sp)) == ' ') {
                    break;
                }
            }
        }

        *sp = *sp+1;
	}

	return value_s;
}

inline int cwobj_parseInt(char **sp) {
	char *value_s = cwobj_parseValue(sp);

    int value = 0;

    if (value_s) {
        value = atoi(value_s);

        free(value_s);
    } else {
        fprintf(stderr, "cwobj_parseInt: cwobj_parseValue failed\n");
    }

	return value;
}

inline float cwobj_parseFloat(char **sp) {
	char *value_s = cwobj_parseValue(sp);

    float value = 0;

    if (value_s) {
	    value = atof(value_s);

	    free(value_s);
    } else {
        fprintf(stderr, "cwobj_parseFloat: cwobj_parseValue failed\n");
    }

	return value;
}

/*
    Generic HASH TABLE implementation (linear probing algorithm)
    It store void pointers (arbitrary data), it is a generic storage solution; you need to free the data you put into it.
*/

#define CWOBJ_HASH_TABLE_INITIAL_SIZE 31

//! CWOBJ hash table data structure
typedef struct {
    unsigned int table_size;
    unsigned int entry_size;

    int lg_table_size;

    char **keys;
    void **vals;
} cwobj_hashtable;

//! create a hashtable
/*!
    \return hashtable data structure pointer
*/
cwobj_hashtable *cwobj_hashTableInit();

//! create a hashtable of a specified size
/*!
    \param size hashtable size
    \return hashtable data structure pointer
*/
cwobj_hashtable *cwobj_hashTableInitExt(unsigned int size);

//! free a hashtable
/*!
    \param hash_table hashtable data structure pointer
*/
void cwobj_hashTableFree(cwobj_hashtable *hash_table);

//! resize a hashtable
/*!
    \param hash_table hashtable data structure pointer
    \param new_size new size
    \return 1 if successfull 0 otherwise
*/
int cwobj_hashTableResize(cwobj_hashtable *hash_table, unsigned int new_size);

//! set a key/value pair
/*!
    \param hash_table hashtable data structure pointer
    \param key key
    \param value arbitrary value
    \return 1 if successfull 0 otherwise
*/
int cwobj_hashTableSet(cwobj_hashtable *hash_table, const char *key, void *value);

//! get a value
/*!
    \param hash_table hashtable data structure pointer
    \param key key
    \return the value associated to the key or NULL
*/
void *cwobj_hashTableGet(cwobj_hashtable *hash_table, const char *key);

//! delete a key with its associated value
/*!
    \param hash_table hashtable data structure pointer
    \param key the key to delete
*/
void cwobj_hashTableDelete(cwobj_hashtable *hash_table, const char *key);

//! show hashtable occupancy statistics and its entire content (stdout)
/*!
    \param hash_table hashtable data structure pointer
*/
void cwobj_hashTableDebug(cwobj_hashtable *hash_table);

//! get a key at a specified index
/*!
    \param hash_table hashtable data structure pointer
    \param index index
    \return key name at specified index or NULL
*/
const char *cwobj_hashTableGetKeyAt(cwobj_hashtable *hash_table, unsigned int index);

//! get a value at a specified index
/*!
    \param hash_table hashtable data structure pointer
    \param index index
    \return the value at specified index or NULL
*/
void *cwobj_hashTableGetValueAt(cwobj_hashtable *hash_table, unsigned int index);

//! get a hash from a given string
/*!
    \param hash_table hashtable data structure pointer
    \param str string used to compute the hash
    \return hash associated to the given string
*/
unsigned int cwobj_hashTableCode(cwobj_hashtable *hash_table, const char *str);

inline unsigned int cwobj_hashTableCode(cwobj_hashtable *hash_table,
                                      const char *str) {
    const int primes[27] = {31, 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381,
                    32749, 65521, 131071, 262139, 524287, 1048573, 2097143,
                    4194301, 8388593, 16777213, 33554393, 67108859, 134217689,
                    268435399, 536870909, 1073741789, 2147483647};

    if (!hash_table || !str) return -1;

    unsigned int hash = 0;

    do {
        hash = (9 * hash + *str) % hash_table->table_size;
    } while (*str++ != '\0');

    if (hash_table->lg_table_size < 26) {
        hash = hash % primes[hash_table->lg_table_size+5];
        return hash % hash_table->table_size;
    }

    return hash;
}

inline cwobj_hashtable *cwobj_hashTableInit() {
	cwobj_hashtable *hash_table = (cwobj_hashtable *)malloc(sizeof(cwobj_hashtable));

	if (!hash_table) return NULL;

    memset(hash_table, 0, sizeof(cwobj_hashtable));

    hash_table->table_size = CWOBJ_HASH_TABLE_INITIAL_SIZE;
    hash_table->lg_table_size = log10(hash_table->table_size);
    hash_table->keys = (char **)malloc(sizeof(char *) * hash_table->table_size);
    hash_table->vals = (void **)malloc(sizeof(void *) * hash_table->table_size);

    memset(hash_table->keys, 0, sizeof(char *) * hash_table->table_size);
    memset(hash_table->vals, 0, sizeof(char *) * hash_table->table_size);

	return hash_table;
}

inline cwobj_hashtable *cwobj_hashTableInitExt(unsigned int size) {
	cwobj_hashtable *hash_table = (cwobj_hashtable *)malloc(sizeof(cwobj_hashtable));

	if (!hash_table) return NULL;

    memset(hash_table, 0, sizeof(cwobj_hashtable));

    hash_table->table_size = size;
    hash_table->lg_table_size = log10(size);
    hash_table->keys = (char **)malloc(sizeof(char *) * size);
    hash_table->vals = (void **)malloc(sizeof(void *) * size);

    memset(hash_table->keys, 0, sizeof(char *) * size);
    memset(hash_table->vals, 0, sizeof(char *) * size);

	return hash_table;
}

inline int cwobj_hashTableResize(cwobj_hashtable *hash_table, unsigned int new_size) {
    if (!hash_table) return 0;

    cwobj_hashtable *ht_tmp = cwobj_hashTableInitExt(new_size);

    char **keys = hash_table->keys;
    void **vals = hash_table->vals;

    unsigned int i = 0;
    for (i = 0; i < hash_table->table_size; i++) {
        cwobj_hashTableSet(ht_tmp, keys[i], vals[i]);
        free(keys[i]);
    }

    free(hash_table->keys);
    free(hash_table->vals);

    hash_table->table_size    = ht_tmp->table_size;
    hash_table->entry_size    = ht_tmp->entry_size;
    hash_table->lg_table_size = ht_tmp->lg_table_size;
    hash_table->keys          = ht_tmp->keys;
    hash_table->vals          = ht_tmp->vals;

    free(ht_tmp);

    return 1;
}

inline int cwobj_hashTableSet(cwobj_hashtable *hash_table,
                                const char *key,
                                void *value) {
    if (!hash_table || !key || !value) return 0;

    if (hash_table->entry_size >= hash_table->table_size / 2) {
        if (!cwobj_hashTableResize(hash_table, 2 * hash_table->table_size)) {
            return 0;
        }
    }

    unsigned int hash = cwobj_hashTableCode(hash_table, key);
    if (hash == (unsigned int)-1) return 0;

    int i = 0;
    for (i = hash; hash_table->keys[i] != NULL;
                                        i = (i + 1) % hash_table->table_size) {
        if (strcmp(hash_table->keys[i], key) == 0) {
            hash_table->vals[i] = value;
            return 1;
        }
    }

    size_t key_length = strlen(key)+1;
    char *key_cpy = (char *)malloc(sizeof(char) * key_length);
    if (!key_cpy) return 0;

    strncpy(key_cpy, key, key_length);

    hash_table->keys[i] = key_cpy;
    hash_table->vals[i] = value;

    hash_table->entry_size++;

    return 1;
}

inline void *cwobj_hashTableGet(cwobj_hashtable *hash_table, const char *key) {

    if (!hash_table || !key) return NULL;

    unsigned int hash = cwobj_hashTableCode(hash_table, key);

    int i = 0;
    for (i = hash; hash_table->keys[i] != NULL;
                                        i = (i + 1) % hash_table->table_size) {
        if (strcmp(hash_table->keys[i], key) == 0) {
            return hash_table->vals[i];
        }
    }

    return NULL;
}

inline void cwobj_hashTableDelete(cwobj_hashtable *hash_table, const char *key) {
    if (!hash_table || !key) return;
    if (!cwobj_hashTableGet(hash_table, key)) return;

    int i = cwobj_hashTableCode(hash_table, key);
    while (strcmp(key, hash_table->keys[i]) != 0) {
        i = (i + 1) % hash_table->table_size;
    }

    free(hash_table->keys[i]);

    hash_table->keys[i] = NULL;
    hash_table->vals[i] = NULL;

    i = (i + 1) % hash_table->table_size;

    while (hash_table->keys[i] != NULL) {
        char *rkey = hash_table->keys[i];
        char *rval = (char *)hash_table->vals[i];
        hash_table->keys[i] = NULL;
        hash_table->vals[i] = NULL;

        hash_table->entry_size--;

        cwobj_hashTableSet(hash_table, rkey, rval);
        i = (i + 1) % hash_table->table_size;
    }

    hash_table->entry_size--;
    if (hash_table->entry_size > 0 ||
        hash_table->entry_size == hash_table->table_size/8) {
        cwobj_hashTableResize(hash_table, hash_table->table_size/2);
    }
}

inline void cwobj_hashTableDebug(cwobj_hashtable *hash_table) {
    if (!hash_table) return;

    fprintf(stdout, ">------- HashTable occupancy pattern <-------\n");
    int wrap = 96;
    unsigned int i = 0;
    for (i = 0; i < hash_table->table_size; i++) {
        if (i%wrap == 0) {
            fprintf(stdout, "\n");
        }

        if (hash_table->keys[i]) {
            fprintf(stdout, "*");
        } else {
            fprintf(stdout, "-");
        }
    }

    fprintf(stdout, "\n\n");
    fprintf(stdout, ">------- HashTable content <-------\n\n");
    for (i = 0; i < hash_table->table_size; i += 1) {
        fprintf(stdout, "key[%i] = %s\n", i, hash_table->keys[i]);
    }
}

inline const char *cwobj_hashTableGetKeyAt(cwobj_hashtable *hash_table,
                                  unsigned int index) {
    if (!hash_table) return NULL;
    if (hash_table->table_size == 0) return NULL;
    if (index >= hash_table->table_size) return NULL;

    return hash_table->keys[index];
}

inline void *cwobj_hashTableGetValueAt(cwobj_hashtable *hash_table, unsigned int index) {
    if (!hash_table) return NULL;
    if (hash_table->table_size == 0) return NULL;
    if (index >= hash_table->table_size) return NULL;

    return hash_table->vals[index];
}

inline void cwobj_hashTableFree(cwobj_hashtable *hash_table) {
    if (!hash_table) return;

    unsigned int i = 0;
    for (i = 0; i < hash_table->table_size; i++) {
        free(hash_table->keys[i]);
    }

    free(hash_table->keys);
    free(hash_table->vals);
    free(hash_table);
}

/*
    OBJ loader

    Support for indices, vertices, texcoords, normals, colors extraction with indices unification (geometry data duplicata removal)

    Limited support for materials; load multiple materials but apply a single material only

    Also support loading of textures automatically but you must provide a callback which will handle the image load and texture creation.

    This was extracted from my old OpenGL engine and enhanced : https://github.com/grz0zrg/M3D
*/

#define CWOBJ_GEOMETRY_POINT    1
#define CWOBJ_GEOMETRY_LINE     2
#define CWOBJ_GEOMETRY_TRIANGLE 4

//! CWOBJ color data structure
typedef struct {
    float r, g, b, unused;
} cwobj_color;

//! CWOBJ material data structure
typedef struct {
    //! material name
    char *name;

    //! ambient color
    cwobj_color ambient_color;
    //! diffuse color
    cwobj_color diffuse_color;
    //! ambient color
    cwobj_color specular_color;
    //! emissive color
    cwobj_color emissive_color;

    //! opacity
    float opacity;

    //! ior
    float ior;
    //! specular hardness
    float specular_hardness;

    //! illumination mode (0 ignore lighting, 1 no specular, 2 lighting)
    int illum_mode;

    //! type
    unsigned int type;
    //! flags
    unsigned int flags;

    //! used flag (unused materials are automatically freed)
    int used;

    //! diffuse texture id
    unsigned int diffuse_texture;
} cwobj_mat;

//! CWOBJ geometry data structure
typedef struct {
    //! indices data
    CWOBJ_INDICE_TYPE *indice;

    //! vertices data
    float *vertice;
    //! texcoords data
    float *texcoord;
    //! uv data
    float *normal;

    //! colors data
    unsigned char *color;

    //! indices count
    unsigned int indice_n;
    //! vertices count (as triplet so you must multiply by 3 to get the data length)
    unsigned int vertice_n;
    //! texcoords count (as pair so you must multiply by 2 to get the data length)
    unsigned int texcoord_n;
    //! normals count (as triplet so you must multiply by 3 to get the data length)
    unsigned int normal_n;
    //! colors count (as triplet so you must multiply by 3 to get the data length)
    unsigned int color_n;

    //! primitive type
    int prim_type;
} cwobj_geo;

//! CWOBJ materials library data structure
typedef struct {
    //! hold materials
    cwobj_mat **materials;

    //! hold the number of stored materials
    unsigned int materials_n;
} cwobj_mtllib;

//! CWOBJ data structure
typedef struct {
    //! mesh name
    char *name;

    //! mesh geometry
    cwobj_geo *geometry;
    //! mesh material
    cwobj_mat *material;
} cwobj;

//! load an .obj file
/*!
    \param filename filename
    \param get_texture function used to load a texture (return a texture id which is stored in the mesh material)
    \return mesh data structure
*/
cwobj *cwobj_load(const char *filename, unsigned int (*get_texture)(const char *filename));

//! create a mesh
/*!
    \return mesh data structure
*/
cwobj *cwobj_meshInit();

//! free a mesh
/*!
    \param mesh mesh data structure
*/
void cwobj_free(cwobj *mesh);

//! set the name of a mesh
/*!
    \param mesh mesh data structure
    \param name mesh name
*/
void cwobj_meshSetName(cwobj *mesh, const char *name);

//! set the material of a mesh
/*!
    \param mesh mesh data structure
    \param material associated material
*/
void cwobj_meshSetMaterial(cwobj *mesh, cwobj_mat *material);

//! set the geometry of a mesh
/*!
    \param mesh mesh data structure
    \param geom associated geometry
*/
void cwobj_meshSetGeometry(cwobj *mesh, cwobj_geo *geom);

//! clone a mesh
/*!
    \param mesh mesh data structure
    \return new mesh data structure
*/
cwobj *cwobj_meshClone(cwobj *mesh);


//! create a geometry
/*!
    \return geometry data structure
*/
cwobj_geo *cwobj_geomInit();

//! free a geometry
/*!
    \param geom geometry data structure
*/
void cwobj_geomFree(cwobj_geo *geom);

//! merge geometry
/*!
    \param dst destination geometry data structure
    \param src source geometry data structure which will be merged into dst
*/
void cwobj_geomMerge(cwobj_geo *dst, cwobj_geo *src);
void cwobj_geomSetVertice(cwobj_geo *geom, unsigned int index, float v1, float v2, float v3);
void cwobj_geomSetTexcoord(cwobj_geo *geom, unsigned int index, float u, float v);
void cwobj_geomAllocVertice(cwobj_geo *geom, unsigned int size);
void cwobj_geomAllocTexcoord(cwobj_geo *geom, unsigned int size);
void cwobj_geomAllocNormal(cwobj_geo *geom, unsigned int size);
void cwobj_geomAllocColor(cwobj_geo *geom, unsigned int size);
void cwobj_geomAllocIndice(cwobj_geo *geom, unsigned int size);
int cwobj_geomAddVertice(cwobj_geo *geom, float v1, float v2, float v3);
int cwobj_geomAddNormal(cwobj_geo *geom, float n1, float n2, float n3);
int cwobj_geomAddTexcoord(cwobj_geo *geom, float u, float v);
int cwobj_geomAddColor(cwobj_geo *geom, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
int cwobj_geomAddIndice(cwobj_geo *geom, CWOBJ_INDICE_TYPE ind);
void cwobj_geomDebug(cwobj_geo *geom);

//! clone a geometry
/*!
    \param geom geometry data structure
    \return new geometry data structure
*/
cwobj_geo *cwobj_geomClone(cwobj_geo *geom);

void cwobj_setColor3f(cwobj_color *color, float r, float g, float b);

cwobj_mat *cwobj_materialInit();
void cwobj_materialFree(cwobj_mat *material);

cwobj_mtllib *cwobj_objMtlibInit();
void cwobj_objMtlibFree(cwobj_mtllib *mtlib);
cwobj_mat *cwobj_objMtlibAddMaterial(cwobj_mtllib *mtlib, char *name);
void cwobj_objMtlibSetUsed(cwobj_mtllib *mtlib, const char *name);
cwobj_mat *cwobj_objMtlibGetMaterial(cwobj_mtllib *mtlib, const char *name);
char *cwobj_objGetLine(FILE *f);
char *cwobj_objGetTag(const char *line);
char *cwobj_objGetData(char *line);
cwobj_mtllib *cwobj_objLoadMtlib(const char *filename, unsigned int (*get_texture)(const char *filename));

inline cwobj *cwobj_meshInit() {
	cwobj *mesh = (cwobj *)malloc(sizeof(cwobj));

	if (!mesh) return NULL;

    memset(mesh, 0, sizeof(cwobj));

	return mesh;
}

inline void cwobj_free(cwobj *mesh) {
    if (!mesh) return;

	free(mesh->name);
	free(mesh);
}

inline void cwobj_meshSetName(cwobj *mesh, const char *name) {
    if (!mesh) return;
    if (!name) return;

    size_t name_length = strlen(name);

    mesh->name = (char *)malloc(sizeof(char) * (name_length+1));
    if (mesh->name) {
        memcpy(mesh->name, name, name_length+1);
    }
}

inline void cwobj_meshSetMaterial(cwobj *mesh, cwobj_mat *material) {
    if (!mesh || !material) return;

    mesh->material = material;
}

inline void cwobj_meshSetGeometry(cwobj *mesh, cwobj_geo *geom) {
    if (!mesh || !geom) return;

    mesh->geometry = geom;
}

inline cwobj *cwobj_meshClone(cwobj *mesh) {
    if (!mesh) return NULL;

    cwobj *new_mesh = cwobj_meshInit();
    if (!new_mesh) return NULL;

    new_mesh->geometry = mesh->geometry;
    new_mesh->material = mesh->material;

    cwobj_meshSetName(new_mesh, mesh->name);

    return new_mesh;
}

inline cwobj_geo *cwobj_geomInit() {
	cwobj_geo *geom = (cwobj_geo *)malloc(sizeof(cwobj_geo));

	if (!geom) return NULL;

    memset(geom, 0, sizeof(cwobj_geo));

	geom->prim_type = CWOBJ_GEOMETRY_TRIANGLE;

	return geom;
}

inline void cwobj_geomFree(cwobj_geo *geom) {
    if (!geom) return;

	free(geom->indice);
	free(geom->vertice);
	free(geom->texcoord);
	free(geom->normal);
	free(geom->color);
	free(geom);
}

inline void cwobj_geomMerge(cwobj_geo *dst, cwobj_geo *src) {
    if (!dst || !src) return;

    unsigned int i = 0, data_length = 0;
    if (dst->indice && src->indice) {
        data_length = (dst->indice_n+src->indice_n);
        CWOBJ_INDICE_TYPE *merged = NULL;
        merged = (CWOBJ_INDICE_TYPE *)realloc(dst->indice,
                                            sizeof(CWOBJ_INDICE_TYPE) *
                                            data_length);

        if (merged) {
            dst->indice = merged;
            for (i = dst->indice_n; i < data_length; i++) {
                dst->indice[i] = src->indice[i-dst->indice_n]+(dst->vertice_n);
            }
            dst->indice_n = data_length;
        }
    }

    if (dst->vertice && src->vertice) {
        data_length = (dst->vertice_n+src->vertice_n) * 3;
        float *merged = NULL;
        merged = (float *)realloc(dst->vertice,
                                            sizeof(float) *
                                            data_length);
        if (merged) {
            dst->vertice = merged;
            unsigned int v_real_n = dst->vertice_n * 3;
            for (i = v_real_n; i < data_length; i++) {
                dst->vertice[i] = src->vertice[i-v_real_n];
            }

            dst->vertice_n = (dst->vertice_n+src->vertice_n);
        }
    }

    if (dst->texcoord && src->texcoord) {
        data_length = (dst->texcoord_n+src->texcoord_n) * 2;
        float *merged = NULL;
        merged = (float *)realloc(dst->texcoord,
                                            sizeof(float) *
                                            data_length);
        if (merged) {
            dst->texcoord = merged;
            unsigned int t_real_n = dst->texcoord_n * 2;
            for (i = t_real_n; i < data_length; i++) {
                dst->texcoord[i] = src->texcoord[i-t_real_n];
            }
            dst->texcoord_n = (dst->texcoord_n+src->texcoord_n);
        }
    }

    if (dst->normal && src->normal) {
        data_length = (dst->normal_n+src->normal_n) * 3;
        float *merged = NULL;
        merged = (float *)realloc(dst->normal,
                                            sizeof(float) *
                                            data_length);
        if (merged) {
            dst->normal = merged;
            unsigned int n_real_n = dst->normal_n * 3;
            for (i = n_real_n; i < data_length; i++) {
                dst->normal[i] = src->normal[i-n_real_n];
            }
            dst->normal_n = (dst->normal_n+src->normal_n);
        }
    }

    if (dst->color && src->color) {
        data_length = (dst->color_n+src->color_n) * 4;
        unsigned char *merged = NULL;
        merged = (unsigned char *)realloc(dst->color,
                                            sizeof(unsigned char) *
                                            data_length);
        if (merged) {
            dst->color = merged;
            unsigned int c_real_n = dst->color_n * 4;
            for (i = c_real_n; i < data_length; i++) {
                dst->color[i] = src->color[i-c_real_n];
            }
            dst->color_n = (dst->color_n+src->color_n);
        }
    }
}

inline void cwobj_geomSetVertice(cwobj_geo *geom, unsigned int index,
                        float v1, float v2, float v3) {
    unsigned int i = index * 3;

    geom->vertice[i]   = v1;
    geom->vertice[i+1] = v2;
    geom->vertice[i+2] = v3;
}

inline void cwobj_geomSetTexcoord(cwobj_geo *geom, unsigned int index,
                         float u, float v) {
    unsigned int i = index<<1;

    geom->texcoord[i]   = u;
    geom->texcoord[i+1] = v;
}

inline void cwobj_geomAllocVertice(cwobj_geo *geom, unsigned int size) {
    if (geom->vertice) free(geom->vertice);

    geom->vertice = (float *)malloc(sizeof(float) * 3 * size);
    geom->vertice_n = size;
}

inline void cwobj_geomAllocTexcoord(cwobj_geo *geom, unsigned int size) {
    if (geom->texcoord) free(geom->texcoord);

    geom->texcoord = (float *)malloc(sizeof(float) * 2 * size);
    geom->texcoord_n = size;
}

inline void cwobj_geomAllocNormal(cwobj_geo *geom, unsigned int size) {
    if (geom->normal) free(geom->normal);

    geom->normal = (float *)malloc(sizeof(float) * 3 * size);
    geom->normal_n = size;
}

inline void cwobj_geomAllocColor(cwobj_geo *geom, unsigned int size) {
    if (geom->color) free(geom->color);

    geom->color = (unsigned char *)malloc(sizeof(unsigned char) * 4 * size);
    geom->color_n = size;
}

inline void cwobj_geomAllocIndice(cwobj_geo *geom, unsigned int size) {
    if (geom->indice) free(geom->indice);

    geom->indice = (CWOBJ_INDICE_TYPE *)malloc(sizeof(CWOBJ_INDICE_TYPE) * size);
    geom->indice_n = size;
}

inline int cwobj_geomAddVertice(cwobj_geo *geom, float v1, float v2, float v3) {
	if (!geom) return 1;

    float *vertice = NULL;
	if (geom->vertice == NULL) {
		vertice = (float *)malloc(sizeof(float) * 3);
	} else {
		vertice = (float *)realloc(geom->vertice,
                                sizeof(float) * 3 * (geom->vertice_n+1));
	}

	if (!vertice) {
		return 1;
	}

	geom->vertice = vertice;

	unsigned int start_index     = geom->vertice_n * 3;
	geom->vertice[start_index]   = v1;
	geom->vertice[start_index+1] = v2;
	geom->vertice[start_index+2] = v3;

	geom->vertice_n++;

	return 0;
}

inline int cwobj_geomAddNormal(cwobj_geo *geom, float n1, float n2, float n3) {
	if (!geom) return 1;

    float *normal = NULL;
	if (geom->normal == NULL) {
		normal = (float *)malloc(sizeof(float) * 3);
	} else {
		normal = (float *)realloc(geom->normal,
                                sizeof(float) * 3 * (geom->normal_n+1));
	}

	if (!normal) {
		return 1;
	}

	geom->normal = normal;

	unsigned int start_index    = geom->normal_n * 3;
	geom->normal[start_index]   = n1;
	geom->normal[start_index+1] = n2;
	geom->normal[start_index+2] = n3;

	geom->normal_n++;

	return 0;
}

inline int cwobj_geomAddTexcoord(cwobj_geo *geom, float u, float v) {
	if (!geom) return 1;

    float *texcoord = NULL;
	if (geom->texcoord == NULL) {
		texcoord = (float *)malloc(sizeof(float) * 2);
	} else {
		texcoord = (float *)realloc(geom->texcoord,
									sizeof(float) * 2 * (geom->texcoord_n+1));
	}

	if (!texcoord) {
		return 1;
	}

	geom->texcoord = texcoord;

	unsigned int start_index    = geom->texcoord_n * 2;
	geom->texcoord[start_index]   = u;
	geom->texcoord[start_index+1] = v;

	geom->texcoord_n++;

	return 0;
}

inline int cwobj_geomAddColor(cwobj_geo *geom, unsigned char r,
                                     unsigned char g,
                                     unsigned char b,
                                     unsigned char a) {
	if (!geom) return 1;

    unsigned char *color = NULL;
	if (geom->color == NULL) {
		color = (unsigned char *)malloc(sizeof(unsigned char) * 4);
	} else {
		color = (unsigned char *)realloc(geom->color,
                                sizeof(unsigned char) * 4 * (geom->color_n+1));
	}

	if (!color) {
		return 1;
	}

	geom->color = color;

	unsigned int start_index   = geom->color_n * 4;
	geom->color[start_index]   = r;
	geom->color[start_index+1] = g;
	geom->color[start_index+2] = b;
	geom->color[start_index+3] = a;

	geom->color_n++;

	return 0;
}

inline int cwobj_geomAddIndice(cwobj_geo *geom, CWOBJ_INDICE_TYPE ind) {
	if (!geom) return 1;

    CWOBJ_INDICE_TYPE *indice = NULL;
	if (geom->indice == NULL) {
		indice = (CWOBJ_INDICE_TYPE *)malloc(sizeof(CWOBJ_INDICE_TYPE));
	} else {
		indice = (CWOBJ_INDICE_TYPE *)realloc(geom->indice,
                                sizeof(CWOBJ_INDICE_TYPE) * (geom->indice_n+1));
	}

	if (!indice) {
		return 1;
	}

	geom->indice = indice;

	geom->indice[geom->indice_n] = ind;

	geom->indice_n++;

	return 0;
}

inline void cwobj_geomDebug(cwobj_geo *geom) {
    if (!geom) return;

    if (!geom->indice) {
        unsigned int i = 0;
        for (i = 0; i < geom->vertice_n*3; i+=3) {
            fprintf(stdout, "Vertice:%f %f %f\n\n",
                  geom->vertice[i], geom->vertice[i+1],
                  geom->vertice[i+2]);
        }

        for (i = 0; i < geom->normal_n*3; i+=3) {
            fprintf(stdout, "Normal:%f %f %f\n\n",
                  geom->normal[i], geom->normal[i+1],
                  geom->normal[i+2]);
        }

        for (i = 0; i < geom->texcoord_n*2; i+=2) {
            fprintf(stdout, "UV:%f %f\n\n",
                  geom->texcoord[i], geom->texcoord[i+1]);
        }
    } else {
        unsigned int i = 0;
        for (i = 0; i < geom->indice_n; i++) {
            unsigned int indice = geom->indice[i];
            fprintf(stdout, "Indice %i:\nVertice:%f %f %f\n"
                   "UV:     %f %f\nNormal: %f %f %f\n\n", indice,
                  geom->vertice[(indice*3)], geom->vertice[(indice*3)+1],
                  geom->vertice[(indice*3)+2],
                  geom->texcoord[(indice*2)], geom->texcoord[(indice*2)+1],
                  geom->normal[(indice*3)], geom->normal[(indice*3)+1],
                  geom->normal[(indice*3)+2]);
        }
    }

    fflush(stdout);
}

inline cwobj_geo *cwobj_geomClone(cwobj_geo *geom) {
    if (!geom) return NULL;

    cwobj_geo *new_geom = cwobj_geomInit();
    if (!new_geom) return NULL;

    cwobj_geomAllocIndice(new_geom, geom->indice_n);
    cwobj_geomAllocVertice(new_geom, geom->vertice_n);
    cwobj_geomAllocNormal(new_geom, geom->normal_n);
    cwobj_geomAllocTexcoord(new_geom, geom->texcoord_n);
    cwobj_geomAllocColor(new_geom, geom->color_n);

    unsigned int i = 0;
    for (i = 0; i < geom->indice_n; i++) {
        new_geom->indice[i] = geom->indice[i];
    }

    for (i = 0; i < geom->vertice_n*3; i++) {
        new_geom->vertice[i] = geom->vertice[i];
    }

    for (i = 0; i < geom->normal_n*3; i++) {
        new_geom->normal[i] = geom->normal[i];
    }

    for (i = 0; i < geom->texcoord_n*2; i++) {
        new_geom->texcoord[i] = geom->texcoord[i];
    }

    for (i = 0; i < geom->color_n*4; i++) {
        new_geom->color[i] = geom->color[i];
    }

    return new_geom;
}


inline void cwobj_setColor3f(cwobj_color *color, float r, float g, float b) {
    color->r = r; color->g = g; color->b = b; color->unused = 0;
}

inline cwobj_mat *cwobj_materialInit() {
	cwobj_mat *material = (cwobj_mat *)malloc(sizeof(cwobj_mat));

	if (!material) return NULL;

	memset(material, 0, sizeof(cwobj_mat));

	return material;
}

inline void cwobj_materialFree(cwobj_mat *material) {
    if (!material) return;

    free(material->name);
	free(material);
}

inline cwobj_mtllib *cwobj_objMtlibInit() {
	cwobj_mtllib *mtlib = (cwobj_mtllib *)malloc(sizeof(cwobj_mtllib));

	if (!mtlib) return NULL;

	memset(mtlib, 0, sizeof(cwobj_mtllib));

	return mtlib;
}

inline cwobj_mat *cwobj_objMtlibAddMaterial(cwobj_mtllib *mtlib, char *name) {
    if (!mtlib) return NULL;

    cwobj_mat *material = cwobj_materialInit();
    if (!material) return NULL;

    material->name = name;

    cwobj_mat **mtls_tmp = NULL;
    if (!mtlib->materials) {
        mtls_tmp = (cwobj_mat **)malloc(sizeof(cwobj_mat *));
    } else {
        mtls_tmp = (cwobj_mat **)realloc(mtlib->materials,
                            sizeof(cwobj_mat *) * (mtlib->materials_n+1));
    }

    if (!mtls_tmp) {
        cwobj_materialFree(material);
        return NULL;
    }

    mtlib->materials = mtls_tmp;

    mtlib->materials[mtlib->materials_n] = material;

    material->used = 0;

    mtlib->materials_n++;

    return material;
}

inline void cwobj_objMtlibSetUsed(cwobj_mtllib *mtlib, const char *name) {
    if (!mtlib || !name) return;

    size_t name_len = strlen(name);

    unsigned int i = 0;
    for (i = 0; i < mtlib->materials_n; i++) {
        cwobj_mat *objmtl = mtlib->materials[i];

        if (!objmtl) continue;

        cwobj_mat *tmp_mat = objmtl;
        const char *material_name = tmp_mat->name;

        if (material_name) {
            if (strlen(material_name) != name_len) {
                continue;
            }

            if (strncmp(name, material_name, name_len) == 0) {
                objmtl->used = 1;
                break;
            }
        }
    }
}

inline cwobj_mat *cwobj_objMtlibGetMaterial(cwobj_mtllib *mtlib, const char *name) {
    if (!name) return NULL;

    cwobj_mat *material = NULL;

    size_t name_len = strlen(name);

    unsigned int i = 0;
    for (i = 0; i < mtlib->materials_n; i++) {
        cwobj_mat *objmtl = mtlib->materials[i];

        if (!objmtl) continue;

        cwobj_mat *tmp_mat = objmtl;
        const char *material_name = tmp_mat->name;

        if (material_name) {
            if (strlen(material_name) != name_len) {
                continue;
            }

            if (strncmp(name, material_name, name_len) == 0) {
                material = tmp_mat;
                break;
            }
        }
    }

    return material;
}

inline void cwobj_objMtlibFree(cwobj_mtllib *mtlib) {
    if (!mtlib) return;

    unsigned int i = 0;
    for (i = 0; i < mtlib->materials_n; i++) {
        cwobj_mat *objmtl = mtlib->materials[i];
        if (!objmtl) continue;

        // only free unused materials
        if (!objmtl->used) {
            cwobj_materialFree(objmtl);
        }
    }
    free(mtlib->materials);
    free(mtlib);
}

inline char *cwobj_objGetLine(FILE *f) {
    if (!f) {
        return NULL;
    }

    char *line = (char *)malloc(sizeof(char) * CWOBJ_MAX_LINE_LENGTH);
    if (!line) {
        return NULL;
    }

    int c = fgetc(f);
    while (c == ' ') { c = fgetc(f); }; // skip whitespaces

    if (c == EOF) {
        free(line);
        return NULL;
    }

    memset(line, 0, sizeof(char) * CWOBJ_MAX_LINE_LENGTH);

    int i = 0; // store true line length
    while (c != '\n' && c != EOF) {
        if (i < (CWOBJ_MAX_LINE_LENGTH-1)) {
            if (c == '#') { // skip .obj comments
                while (c != '\n' && c != EOF) { c = fgetc(f); };
                break;
            }

            line[i++] = c;
        }

        c = fgetc(f);
    }

    return line;
}

inline char *cwobj_objGetTag(const char *line) {
    if (!line) return NULL;

    long line_length = strlen(line);

    if ((line_length == 1 && line[0] == 0x0D) || (line_length == 1 && line[0] == 0x0A)) return NULL;
    if (line_length == 2 && line[0] == 0x0D && line[1] == 0x0A) return NULL;

    int tag_end = strcspn(line, " ");
    char *tag = (char *)malloc(sizeof(char) * (tag_end+1));
    if (!tag) return NULL;

    strncpy(tag, line, tag_end);
    tag[tag_end] = '\0';

    cwobj_toLowercase(tag);

    return tag;
}

inline char *cwobj_objGetData(char *line) {
    if (!line) return NULL;

    int data_start = strcspn(line, " ")+1;
    size_t data_length = strlen(line)-data_start;

    char *data_start_p = line+data_start;

    // skip whitespace
    while (*data_start_p == ' ') { data_start_p++; data_length--; };

    if (data_length <= 0) return NULL;

    char *data = (char *)malloc(sizeof(char) * (data_length+1));
    if (!data) return NULL;

    memcpy(data, data_start_p, data_length+1);

    return data;
}

inline cwobj_mtllib *cwobj_objLoadMtlib(const char *filename, unsigned int (*get_texture)(const char *filename)) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "cwobj_objLoadMtlib: failed to open .mtl '%s'\n", filename);
        return NULL;
    }

    cwobj_mtllib *mtlib = cwobj_objMtlibInit();
    if (!mtlib) {
        fprintf(stderr, "cwobj_objLoadMtlib: cwobj_objMtlibInit failed\n");
        return NULL;
    }

    cwobj_mat *curr_material = NULL;

	// parse the .mtl
    long curr_line = 0;
    char *line = NULL;
    while ((line = cwobj_objGetLine(f))) {
        unsigned int line_length = strlen(line);

        curr_line += 1;

		if (!line_length) {
            free(line);
            continue;
		}

        char *tag = cwobj_objGetTag(line);
        if (!tag) {
            free(line);
            continue;
        }

        int tag_length = strlen(tag);
        if (tag_length == 0) {
            free(line);
            free(tag); 
            continue;
        }

        char *data = cwobj_objGetData(line);
        if (!data) {
            free(line);
            free(tag);
            continue;
        }

        char *data_p = data;

        int data_length = strlen(data);

#ifdef DEBUG
        fprintf(stdout, "cwobj_objLoadMtlib: '%s' L%lu parsing tag '%s' with value '%s'\n", filename, curr_line, tag, data);
#endif

        if (cwobj_strncmpi(tag, "newmtl", 6) == 0) {
            char *name = (char *)malloc(sizeof(char) * (data_length+1));
            if (name) {
                memcpy(name, data, data_length+1);
                
                curr_material = cwobj_objMtlibAddMaterial(mtlib, name);
                if (!curr_material) {
                    free(name);
                    fprintf(stderr, "cwobj_objLoadMtlib: (newmtl) failed to add material '%s' to the mtlib\n", filename);
                }
            } else {
                fprintf(stderr, "cwobj_objLoadMtlib: (newmtl) failed to allocate material name '%s'\n", filename);
            }
        } else if (cwobj_strncmpi(tag, "map_kd", 6) == 0) {
            char *path = cwobj_getPath(filename);

            size_t path_length = 0;

            if (path) {
                path_length = strlen(path);
            }

            char *texture_filename = (char *)malloc(sizeof(char) *
                                            (data_length+path_length+1));
            if (texture_filename) {
                if (path) {
                    memcpy(texture_filename, path, path_length);
                    memcpy(texture_filename+path_length, data, data_length+1);
                } else {
                    memcpy(texture_filename, data, data_length+1);
                }

                if (curr_material) {
                    if (get_texture) {
                        curr_material->diffuse_texture = get_texture(texture_filename);
                    } else {
                        fprintf(stderr, "cwobj_objLoadMtlib: texture '%s' ignored (no callback registered to load the texture)\n", texture_filename);
                    }
                }

                free(texture_filename);
            } else {
                fprintf(stderr, "cwobj_objLoadMtlib: '%s' ignored (malloc. error)\n", tag);
            }

            free(path);
        } else if (cwobj_strncmpi(tag, "ns" , 2) == 0) {
            if (curr_material) {
                int specular_hardness = cwobj_parseInt(&data_p);

                curr_material->specular_hardness = specular_hardness;
            }
        } else if (cwobj_strncmpi(tag, "ka" , 2) == 0) {
            if (curr_material) {
                float r = cwobj_parseFloat(&data_p);
                float g = cwobj_parseFloat(&data_p);
                float b = cwobj_parseFloat(&data_p);

                cwobj_setColor3f(&curr_material->ambient_color, r, g, b);
            }
        } else if (cwobj_strncmpi(tag, "ke" , 2) == 0) {
            if (curr_material) {
                float r = cwobj_parseFloat(&data_p);
                float g = cwobj_parseFloat(&data_p);
                float b = cwobj_parseFloat(&data_p);

                cwobj_setColor3f(&curr_material->emissive_color, r, g, b);
            }
        } else if (cwobj_strncmpi(tag, "kd" , 2) == 0) {
            if (curr_material) {
                float r = cwobj_parseFloat(&data_p);
                float g = cwobj_parseFloat(&data_p);
                float b = cwobj_parseFloat(&data_p);

                cwobj_setColor3f(&curr_material->diffuse_color, r, g, b);
            }
        } else if (cwobj_strncmpi(tag, "ks" , 2) == 0) {
            if (curr_material) {
                float r = cwobj_parseFloat(&data_p);
                float g = cwobj_parseFloat(&data_p);
                float b = cwobj_parseFloat(&data_p);

                cwobj_setColor3f(&curr_material->specular_color, r, g, b);
            }
        } else if (cwobj_strncmpi(tag, "ni" , 2) == 0) {
            if (curr_material) {
                float ior = cwobj_parseFloat(&data_p);

                curr_material->ior = ior;
            }
        } else if (cwobj_strncmpi(tag, "d" , 1) == 0) {
            if (curr_material) {
                float opacity = cwobj_parseFloat(&data_p);

                curr_material->opacity = opacity;
            }
        } else if (cwobj_strncmpi(tag, "illum", 5) == 0) {
            if (curr_material) {
                int illum_mode = cwobj_parseInt(&data_p);

                curr_material->illum_mode = illum_mode;
            }
        } else {
            fprintf(stderr, "cwobj_objLoadMtlib: unknow .mtl tag: '%s'\n", tag);
        }

        free(line);
        free(tag);
        free(data);
    }

    fclose(f);

    return mtlib;
}

inline cwobj *cwobj_load(const char *filename, unsigned int (*get_texture)(const char *filename)) {
    fprintf(stdout, "cwobj_load: loading \"%s\"\n", filename);

    fflush(stdout);

    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "cwobj_load: failed to open .obj \"%s\"\n", filename);
        return NULL;
    }

	cwobj *mesh = cwobj_meshInit();
	if (!mesh) {
        fprintf(stderr, "cwobj_load: failed to open .obj \"%s\" (mesh. init. failed)\n", filename);
        return NULL;
	}

	cwobj_geo *obj_geom = cwobj_geomInit(); // store temporary obj data
	if (!obj_geom) {
        fprintf(stderr, "cwobj_load: failed to open .obj \"%s\" (geom. init. failed)\n", filename);
        cwobj_free(mesh);
        fclose(f);
        return NULL;
	}

	cwobj_geo *vcwobj_geom = cwobj_geomInit(); // store final geometry
	if (!vcwobj_geom) {
        fprintf(stderr, "cwobj_load: failed to open .obj \"%s\" (geom. init. failed)\n", filename);
        cwobj_geomFree(obj_geom);
        cwobj_free(mesh);
        fclose(f);
        return NULL;
	}

    // will store the associated .mtl file data
	cwobj_mtllib *mtlib = NULL;

	/* will be used to unify indices to optimize the final geometry with
       zero geometry duplicata, with a hash table it is simple but it may
       require huge amount of memory...
	*/
	cwobj_hashtable *indices_hash_table = cwobj_hashTableInit();

	// .obj can store many meshs
	int mesh_count = 0;

    char *line = NULL;
    long curr_line = 0;
    while ((line = cwobj_objGetLine(f))) {
        unsigned int line_length = strlen(line);

        curr_line += 1;

		if (!line_length) {
            free(line);
            continue;
		}

        char *tag = cwobj_objGetTag(line);
        if (!tag) {
            free(line);
            continue;
        }

        int tag_length = strlen(tag);
        if (tag_length == 0) {
            free(line);
            free(tag); 
            continue;
        }

        char *data = cwobj_objGetData(line);
        if (!data) {
            free(line);
            free(tag);
            continue;
        }

        char *data_p = data;

        int data_length = strlen(data);

#ifdef DEBUG
        fprintf(stdout, "cwobj_load: '%s' L%lu parsing tag '%s' with value '%s'\n", filename, curr_line, tag, data);
#endif

		// parse tags
        if (strncmp(tag, "mtllib", 6) == 0) {
            char *path = cwobj_getPath(filename);

            size_t path_length = 0;

            if (path) {
                path_length = strlen(path);
            }

            char *mtl_filename = (char *)malloc(sizeof(char) *
                                                (data_length+path_length+1));
            if (mtl_filename) {
                if (path) {
                    memcpy(mtl_filename, path, path_length);
                    memcpy(mtl_filename+path_length, data, data_length+1);
                } else {
                    memcpy(mtl_filename, data, data_length+1);
                }

                mtlib = cwobj_objLoadMtlib(mtl_filename, get_texture);
                if (!mtlib) {
                    fprintf(stderr, "cwobj_load: failed to load .mtl \"%s\"\n", mtl_filename);
                }

                free(mtl_filename);
            } else {
                fprintf(stderr, "cwobj_load: \"%s\" ignored (malloc. error)\n", tag);
            }

            free(path);
        } else if (strncmp(tag, "usemtl", 6) == 0) {
            if (!mesh->material && mtlib) {
                cwobj_mat *material = cwobj_objMtlibGetMaterial(mtlib, data);
                if (!material) {
                    fprintf(stderr, "cwobj_load: cannot find material in the mtllib \"%s\" \n", data);
                } else {
                    cwobj_meshSetMaterial(mesh, material);
                }
            } else {
                fprintf(stderr, "cwobj_load: \"%s\" ignored (a material is already bound to a mesh or no mtllib present)\n", tag);
            }
        } else if (strncmp(tag, "l", 1) == 0) { // edges
            vcwobj_geom->prim_type = CWOBJ_GEOMETRY_LINE;
        } else if (strncmp(tag, "f", 1) == 0) {  // face

            long iface_type = 0;
            char **strings_list = cwobj_splitString(data, ' ', &iface_type);

            int i = 0;
            for (i = 0; i < iface_type; i++) {
                char *indices = strings_list[i];
                char **indices_list = NULL;
                long ind_list_length = 0;

                indices_list = cwobj_splitString(indices, '/',
                                                   &ind_list_length);
                if (!indices_list || ind_list_length == 0) {
                    continue;
                }

                if (ind_list_length > 3) {
                    fprintf(stderr, "cwobj_load: \"%s\" warning: unknow face type\n", filename);
                    continue;
                }

                unsigned int *indice = NULL;
                indice = (unsigned int *)cwobj_hashTableGet(indices_hash_table,
                                                          indices);

                if (indice != NULL) { // the data for that indice exist already!
                    if (cwobj_geomAddIndice(vcwobj_geom, *indice)) {
                        fprintf(stderr, "cwobj_load: \"%s\" failed (cwobj_geomAddIndice error)\n", filename);
                        cwobj_free(mesh);

                        fclose(f);
                        break;
                    }
                } else { // the indice does not so we add it with the geom data
                    unsigned int vertice_indice = atoi(indices_list[0]);
                    unsigned int vertice_index  = (vertice_indice-1) * 3;

                    if (vertice_indice > obj_geom->vertice_n) continue;

                    float v1 = obj_geom->vertice[vertice_index];
                    float v2 = obj_geom->vertice[vertice_index+1];
                    float v3 = obj_geom->vertice[vertice_index+2];

                    if (cwobj_geomAddVertice(vcwobj_geom, v1, v2, v3)) {
                        fprintf(stderr, "cwobj_load: \"%s\" failed (cwobj_geomAddVertice error)\n", filename);
                        cwobj_free(mesh);

                        fclose(f);
                        break;
                    }
                    if (cwobj_geomAddIndice(vcwobj_geom, vcwobj_geom->vertice_n-1)) {
                        fprintf(stderr, "cwobj_load: \"%s\" failed (cwobj_geomAddIndice error)\n", filename);
                        cwobj_free(mesh);

                        fclose(f);
                        break;
                    }

                    if (ind_list_length > 1) {
                        unsigned int uv_indice = atoi(indices_list[1]);
                        if (uv_indice > 0) {
                            unsigned int uv_index = (uv_indice-1) * 2;

                            if (uv_indice > obj_geom->texcoord_n) continue;

                            float u = obj_geom->texcoord[uv_index];
                            float v = obj_geom->texcoord[uv_index + 1];

                            if (cwobj_geomAddTexcoord(vcwobj_geom, u, v)) {
                                fprintf(stderr, "cwobj_load: \"%s\" failed (cwobj_geomAddTexcoord error)\n", filename);
                                cwobj_free(mesh);

                                fclose(f);
                                break;
                            }
                        }
                    }

                    if (ind_list_length == 3) {
                        unsigned int normal_indice = atoi(indices_list[2]);
                        unsigned int normal_index = (normal_indice-1) * 3;

                        if (normal_indice > obj_geom->normal_n) continue;

                        float n1 = obj_geom->normal[normal_index];
                        float n2 = obj_geom->normal[normal_index + 1];
                        float n3 = obj_geom->normal[normal_index + 2];

                        if (cwobj_geomAddNormal(vcwobj_geom, n1, n2, n3)) {
                            fprintf(stderr, "cwobj_load: \"%s\" failed (cwobj_geomAddNormal error)\n", filename);
                            cwobj_free(mesh);

                            fclose(f);
                            break;
                        }
                    }

                    // we add the indices to the hash table
                    unsigned int *new_indice = (unsigned int *)malloc(
                                                        sizeof(unsigned int));
                    *new_indice = vcwobj_geom->vertice_n-1;
                    cwobj_hashTableSet(indices_hash_table, indices,
                                     new_indice);
                }

                cwobj_freeSplitResult(indices_list, ind_list_length);
            }
            cwobj_freeSplitResult(strings_list, iface_type);

            if (iface_type == 3) {
                vcwobj_geom->prim_type = CWOBJ_GEOMETRY_TRIANGLE;
            } else {
                fprintf(stderr, "cwobj_load: \"%s\" failed (unsupported face type)\n", filename);
                cwobj_free(mesh);
                fclose(f);
            }
        } else if (strncmp(tag, "s", 1) == 0) {  // smooth
            if (*data == '1') {

            } else { // flat

            }
        } else if (strncmp(tag, "o", 1) == 0) {  // object
            mesh_count++;
            if (mesh_count > 1) { // only one is loaded
                fclose(f);
            }

            cwobj_meshSetName(mesh, data);
        } else if (*tag == 'v' && tag_length == 1) {  // vertice
            float v1 = cwobj_parseFloat(&data_p);
            float v2 = cwobj_parseFloat(&data_p);
            float v3 = cwobj_parseFloat(&data_p);

            if (cwobj_geomAddVertice(obj_geom, v1, v2, v3)) {
                fprintf(stderr, "cwobj_load: \"%s\" failed (cwobj_geomAddVertice error)\n", filename);

                cwobj_free(mesh);
				fclose(f);
            }
        } else if (strncmp(tag, "vn", 2) == 0) { // normal
            float n1 = cwobj_parseFloat(&data_p);
            float n2 = cwobj_parseFloat(&data_p);
            float n3 = cwobj_parseFloat(&data_p);

            if (cwobj_geomAddNormal(obj_geom, n1, n2, n3)) {
                fprintf(stderr, "cwobj_load: \"%s\" failed (cwobj_geomAddNormal error)\n", filename);

                cwobj_free(mesh);
				fclose(f);
            }
        } else if (strncmp(tag, "vt", 2) == 0) { // texcoord
            float u = cwobj_parseFloat(&data_p);
            float v = cwobj_parseFloat(&data_p);

            if (cwobj_geomAddTexcoord(obj_geom, u, v)) {
                fprintf(stderr, "cwobj_load: \"%s\" failed (cwobj_geomAddTexcoord error)\n", filename);

                cwobj_free(mesh);
                fclose(f);
            }
        } else {
            fprintf(stderr, "cwobj_load: unknow .obj tag: \"%s\"\n", tag);
        }

		free(line);
        free(tag);
        free(data);
    }

    cwobj_geomFree(obj_geom);

    if (!mesh) {
        cwobj_geomFree(vcwobj_geom);
    } else {
        if (vcwobj_geom->indice_n == 0) {
            vcwobj_geom->prim_type = CWOBJ_GEOMETRY_POINT;
        }

        cwobj_meshSetGeometry(mesh, vcwobj_geom);

        // we set it as used in the mtllib so cwobj_objMtlibFree does not free it
        if (mesh->material) {
            cwobj_objMtlibSetUsed(mtlib, mesh->material->name);
        }
    }

    // free the elements of the hash table
    unsigned int i = 0;
    for (i = 0; i < indices_hash_table->table_size; i++) {
        const char *key = cwobj_hashTableGetKeyAt(indices_hash_table, i);
        if (!key) continue;

        free(cwobj_hashTableGet(indices_hash_table, key));
    }
    cwobj_hashTableFree(indices_hash_table);

    cwobj_objMtlibFree(mtlib);

    fclose(f);

    return mesh;
}

#endif