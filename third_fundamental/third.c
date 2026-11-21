
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_FILE_OPEN_ERROR,
    STATUS_FILE_READ_ERROR,
    STATUS_FILE_CLOSE_ERROR,
    STATUS_MEMORY_ERROR,
    STATUS_OVERFLOW_ERROR,
    STATUS_HEAP_EMPTY,
    STATUS_HEAP_FULL,
    STATUS_OUTPUT_ERROR
} Status;

typedef struct HuffmanNode {
    unsigned char value;
    unsigned long long frequency;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

typedef struct {
    HuffmanNode **nodes;
    size_t size;
    size_t capacity;
} MinHeap;


typedef struct {
    HuffmanNode *nodes;
    HuffmanNode *root;
    size_t count;
} HuffmanTree;


typedef struct {
    unsigned long long original_bytes;
    unsigned long long encoded_bits;
    unsigned long long encoded_bytes;
    long double savings_percent;
} CompressionStats;



Status heapInit(MinHeap *heap, size_t capacity) {
    HuffmanNode **nodes;

    if (heap == NULL || capacity == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (heap->nodes != NULL || heap->size != 0 || heap->capacity != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (capacity > SIZE_MAX / sizeof(*nodes)) {
        return STATUS_OVERFLOW_ERROR;
    }

    nodes = (HuffmanNode **)malloc(capacity * sizeof(*nodes));

    if (nodes == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    heap->nodes = nodes;
    heap->size = 0;
    heap->capacity = capacity;

    return STATUS_OK;
}



Status heapPush(MinHeap *heap, HuffmanNode *node) {
    size_t index;
    size_t parent;
    HuffmanNode *temp;

    if (heap == NULL || node == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (heap->nodes == NULL || heap->capacity == 0 || heap->size > heap->capacity || node->frequency == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (heap->size == heap->capacity) {
        return STATUS_HEAP_FULL;
    }

    index = heap->size;
    heap->nodes[index] = node;
    heap->size++;

    while (index > 0) {
        parent = (index - 1) / 2;

        if (heap->nodes[parent]->frequency <= heap->nodes[index]->frequency) {
            break;
        }

        temp = heap->nodes[parent];
        heap->nodes[parent] = heap->nodes[index];
        heap->nodes[index] = temp;

        index = parent;
    }

    return STATUS_OK;
}



Status heapPop(MinHeap *heap, HuffmanNode **result) {
    HuffmanNode *minimum;
    HuffmanNode *temp;
    size_t index;
    size_t left;
    size_t right;
    size_t smallest;

    if (heap == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (heap->nodes == NULL || heap->capacity == 0 || heap->size > heap->capacity) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (heap->size == 0) {
        return STATUS_HEAP_EMPTY;
    }

    minimum = heap->nodes[0];
    heap->size--;

    if (heap->size == 0) {
        heap->nodes[0] = NULL;
        *result = minimum;
        return STATUS_OK;
    }

    heap->nodes[0] = heap->nodes[heap->size];
    heap->nodes[heap->size] = NULL;

    index = 0;

    while (index < heap->size / 2) {
        left = 2 * index + 1;
        right = left + 1;
        smallest = left;

        if (right < heap->size && heap->nodes[right]->frequency < heap->nodes[left]->frequency) {
            smallest = right;
        }

        if (heap->nodes[index]->frequency <= heap->nodes[smallest]->frequency) {
            break;
        }

        temp = heap->nodes[index];
        heap->nodes[index] = heap->nodes[smallest];
        heap->nodes[smallest] = temp;

        index = smallest;
    }

    *result = minimum;

    return STATUS_OK;
}


Status heapFree(MinHeap *heap) {
    if (heap == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    free(heap->nodes);

    heap->nodes = NULL;
    heap->size = 0;
    heap->capacity = 0;

    return STATUS_OK;
}



Status countBufferFrequencies(const unsigned char *buffer, size_t length, unsigned long long frequencies[256]) {
    size_t i;
    unsigned char value;

    if (frequencies == NULL || (buffer == NULL && length != 0)) {
        return STATUS_INVALID_ARGUMENT;
    }

    for (i = 0; i < length; i++) {
        value = buffer[i];

        if (frequencies[value] == ULLONG_MAX) {
            return STATUS_OVERFLOW_ERROR;
        }

        frequencies[value]++;
    }

    return STATUS_OK;
}



Status readFrequencies(const char *path, unsigned long long frequencies[256], unsigned long long *file_size) {
    FILE *file;
    unsigned char buffer[4096];
    unsigned long long local_frequencies[256] = {0};
    unsigned long long total = 0;
    size_t bytes_read;
    size_t i;
    Status status = STATUS_OK;

    if (path == NULL || path[0] == '\0' ||
        frequencies == NULL || file_size == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    file = fopen(path, "rb");

    if (file == NULL) {
        return STATUS_FILE_OPEN_ERROR;
    }

    while (status == STATUS_OK) {
        bytes_read = fread(buffer, 1, sizeof(buffer), file);

        if ((unsigned long long)bytes_read > ULLONG_MAX - total) {
            status = STATUS_OVERFLOW_ERROR;
            break;
        }

        status = countBufferFrequencies(buffer, bytes_read, local_frequencies);

        if (status != STATUS_OK) {
            break;
        }

        total += (unsigned long long)bytes_read;

        if (bytes_read < sizeof(buffer)) {
            if (ferror(file)) {
                status = STATUS_FILE_READ_ERROR;
            }
            break;
        }
    }

    if (fclose(file) != 0 && status == STATUS_OK) {
        status = STATUS_FILE_CLOSE_ERROR;
    }

    if (status != STATUS_OK) {
        return status;
    }

    for (i = 0; i < 256; i++) {
        frequencies[i] = local_frequencies[i];
    }

    *file_size = total;

    return STATUS_OK;
}



Status buildHuffmanTree(const unsigned long long frequencies[256], HuffmanTree *tree) {
    size_t distinct = 0;
    size_t capacity;
    size_t count = 0;
    size_t i;
    HuffmanNode *nodes;
    HuffmanNode *left;
    HuffmanNode *right;
    HuffmanNode *parent;
    HuffmanNode *root = NULL;
    MinHeap heap = {0};
    Status status;
    Status cleanup_status;

    if (frequencies == NULL || tree == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (tree->nodes != NULL || tree->root != NULL ||
        tree->count != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    for (i = 0; i < 256; i++) {
        if (frequencies[i] != 0) {
            distinct++;
        }
    }

    if (distinct == 0) {
        return STATUS_OK;
    }

    capacity = 2 * distinct - 1;

    if (capacity > SIZE_MAX / sizeof(*nodes)) {
        return STATUS_OVERFLOW_ERROR;
    }

    nodes = (HuffmanNode *)malloc(capacity * sizeof(*nodes));

    if (nodes == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    status = heapInit(&heap, distinct);

    if (status != STATUS_OK) {
        free(nodes);
        return status;
    }

    for (i = 0; i < 256; i++) {
        if (frequencies[i] == 0) {
            continue;
        }

        nodes[count].value = (unsigned char)i;
        nodes[count].frequency = frequencies[i];
        nodes[count].left = NULL;
        nodes[count].right = NULL;

        status = heapPush(&heap, &nodes[count]);

        if (status != STATUS_OK) {
            break;
        }

        count++;
    }

    while (status == STATUS_OK && heap.size > 1) {
        status = heapPop(&heap, &left);

        if (status != STATUS_OK) {
            break;
        }

        status = heapPop(&heap, &right);

        if (status != STATUS_OK) {
            break;
        }

        if (left->frequency > ULLONG_MAX - right->frequency) {
            status = STATUS_OVERFLOW_ERROR;
            break;
        }

        parent = &nodes[count];

        parent->value = 0;
        parent->frequency = left->frequency + right->frequency;
        parent->left = left;
        parent->right = right;

        status = heapPush(&heap, parent);

        if (status != STATUS_OK) {
            break;
        }

        count++;
    }

    if (status == STATUS_OK) {
        status = heapPop(&heap, &root);
    }

    cleanup_status = heapFree(&heap);

    if (status == STATUS_OK) {
        status = cleanup_status;
    }

    if (status != STATUS_OK) {
        free(nodes);
        return status;
    }

    tree->nodes = nodes;
    tree->root = root;
    tree->count = count;

    return STATUS_OK;
}



Status generateCodesRecursive(const HuffmanNode *node, char codes[256][256], char path[256], size_t depth) {
    size_t i;
    Status status;

    if (node == NULL || codes == NULL || path == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (node->left == NULL && node->right == NULL) {
        if (depth > 255) {
            return STATUS_INVALID_ARGUMENT;
        }

        if (depth == 0) {
            codes[node->value][0] = '0';
            codes[node->value][1] = '\0';
            return STATUS_OK;
        }

        for (i = 0; i < depth; i++) {
            codes[node->value][i] = path[i];
        }

        codes[node->value][depth] = '\0';

        return STATUS_OK;
    }

    if (node->left == NULL || node->right == NULL || depth >= 255) {
        return STATUS_INVALID_ARGUMENT;
    }

    path[depth] = '0';

    status = generateCodesRecursive(node->left, codes, path, depth + 1);

    if (status != STATUS_OK) {
        return status;
    }

    path[depth] = '1';

    return generateCodesRecursive(node->right, codes, path, depth + 1);
}


Status generateCodes(const HuffmanTree *tree, char codes[256][256]) {
    char local_codes[256][256] = {{0}};
    char path[256];
    Status status;

    if (tree == NULL || codes == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (tree->root == NULL) {
        if (tree->nodes != NULL || tree->count != 0) {
            return STATUS_INVALID_ARGUMENT;
        }
    } else {
        if (tree->nodes == NULL || tree->count == 0 || tree->count > 511) {
            return STATUS_INVALID_ARGUMENT;
        }

        status = generateCodesRecursive(tree->root, local_codes, path, 0);

        if (status != STATUS_OK) {
            return status;
        }
    }

    memcpy(codes, local_codes, sizeof(local_codes));

    return STATUS_OK;
}


Status freeHuffmanTree(HuffmanTree *tree) {
    if (tree == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    free(tree->nodes);

    tree->nodes = NULL;
    tree->root = NULL;
    tree->count = 0;

    return STATUS_OK;
}



Status calculateStatistics(const unsigned long long frequencies[256], const char codes[256][256], unsigned long long file_size, CompressionStats *stats) {
    CompressionStats result = {0};
    unsigned long long total_frequency = 0;
    unsigned long long bits;
    size_t i;
    size_t length;

    if (frequencies == NULL || codes == NULL || stats == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    result.original_bytes = file_size;

    for (i = 0; i < 256; i++) {
        if (frequencies[i] > file_size - total_frequency) {
            return STATUS_INVALID_ARGUMENT;
        }

        total_frequency += frequencies[i];

        if (frequencies[i] == 0) {
            if (codes[i][0] != '\0') {
                return STATUS_INVALID_ARGUMENT;
            }
            continue;
        }

        length = 0;

        while (length < 256 && codes[i][length] != '\0') {
            if (codes[i][length] != '0' && codes[i][length] != '1') {
                return STATUS_INVALID_ARGUMENT;
            }

            length++;
        }

        if (length == 0 || length == 256) {
            return STATUS_INVALID_ARGUMENT;
        }

        if (frequencies[i] > ULLONG_MAX / length) {
            return STATUS_OVERFLOW_ERROR;
        }

        bits = frequencies[i] * length;

        if (bits > ULLONG_MAX - result.encoded_bits) {
            return STATUS_OVERFLOW_ERROR;
        }

        result.encoded_bits += bits;
    }

    if (total_frequency != file_size) {
        return STATUS_INVALID_ARGUMENT;
    }

    result.encoded_bytes = result.encoded_bits / 8;

    if (result.encoded_bits % 8 != 0) {
        result.encoded_bytes++;
    }

    if (file_size == 0) {
        result.savings_percent = 0.0L;
    } else {
        result.savings_percent =
            100.0L * (1.0L -
            (long double)result.encoded_bytes /
            (long double)file_size);
    }

    *stats = result;

    return STATUS_OK;
}



Status printResults(const unsigned long long frequencies[256], const char codes[256][256], const CompressionStats *stats) {
    size_t i;
    size_t j;

    if (frequencies == NULL || codes == NULL || stats == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    for (i = 0; i < 256; i++) {
        if (frequencies[i] == 0) {
            if (codes[i][0] != '\0') {
                return STATUS_INVALID_ARGUMENT;
            }
            continue;
        }

        for (j = 0; j < 256; j++) {
            if (codes[i][j] == '\0') {
                break;
            }

            if (codes[i][j] != '0' && codes[i][j] != '1') {
                return STATUS_INVALID_ARGUMENT;
            }
        }

        if (j == 0 || j == 256) {
            return STATUS_INVALID_ARGUMENT;
        }
    }

    if (fprintf(stdout, "Byte\tFrequency\tCode\n") < 0) {
        return STATUS_OUTPUT_ERROR;
    }

    for (i = 0; i < 256; i++) {
        if (frequencies[i] == 0) {
            continue;
        }

        if (fprintf(stdout, "%zu\t%llu\t%s\n",
                    i, frequencies[i], codes[i]) < 0) {
            return STATUS_OUTPUT_ERROR;
        }
    }

    if (fprintf(stdout,
                "\nOriginal size: %llu bytes\n"
                "Encoded size: %llu bits\n"
                "Encoded size: %llu bytes\n"
                "Savings: %.2Lf%%\n",
                stats->original_bytes,
                stats->encoded_bits,
                stats->encoded_bytes,
                stats->savings_percent) < 0) {
        return STATUS_OUTPUT_ERROR;
    }

    if (fflush(stdout) == EOF) {
        return STATUS_OUTPUT_ERROR;
    }

    return STATUS_OK;
}



int main(int argc, char *argv[]) {
    unsigned long long frequencies[256] = {0};
    unsigned long long file_size = 0;
    HuffmanTree tree = {0};
    char codes[256][256] = {{0}};
    CompressionStats stats = {0};
    const char (*read_only_codes)[256] = (const char (*)[256])codes;
    Status status;
    Status cleanup_status;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    status = readFrequencies(argv[1], frequencies, &file_size);

    if (status == STATUS_OK) {
        status = buildHuffmanTree(frequencies, &tree);
    }

    if (status == STATUS_OK) {
        status = generateCodes(&tree, codes);
    }

    if (status == STATUS_OK) {
        status = calculateStatistics(frequencies, read_only_codes, file_size, &stats);
    }

    if (status == STATUS_OK) {
        status = printResults(frequencies, read_only_codes, &stats);
    }

    cleanup_status = freeHuffmanTree(&tree);

    if (status == STATUS_OK) {
        status = cleanup_status;
    }

    if (status != STATUS_OK) {
        if (status == STATUS_INVALID_ARGUMENT) {
            fprintf(stderr, "Error: invalid argument.\n");
        } else if (status == STATUS_FILE_OPEN_ERROR) {
            fprintf(stderr, "Error: cannot open file.\n");
        } else if (status == STATUS_FILE_READ_ERROR) {
            fprintf(stderr, "Error: cannot read file.\n");
        } else if (status == STATUS_FILE_CLOSE_ERROR) {
            fprintf(stderr, "Error: cannot close file.\n");
        } else if (status == STATUS_MEMORY_ERROR) {
            fprintf(stderr, "Error: out of memory.\n");
        } else if (status == STATUS_OVERFLOW_ERROR) {
            fprintf(stderr, "Error: integer overflow.\n");
        } else if (status == STATUS_HEAP_EMPTY) {
            fprintf(stderr, "Error: heap is empty.\n");
        } else if (status == STATUS_HEAP_FULL) {
            fprintf(stderr, "Error: heap is full.\n");
        } else if (status == STATUS_OUTPUT_ERROR) {
            fprintf(stderr, "Error: cannot write output.\n");
        } else {
            fprintf(stderr, "Error: unknown status.\n");
        }

        return 1;
    }

    return 0;
}
