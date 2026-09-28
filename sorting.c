/*
    Program Description: This program processes production data for dispatching car parts.
                         1. Sorts products by weight using Merge Sort.
                         2. Merges four batch files into one dispatch list.
                         3. Searches for a product by weight using Binary Search.
                         4. Calculates the items, weight and price for a van of up to 100 kg.

    Author: Danas Savickas
    Date: 16/04/2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_PRODUCTS 1000
#define LINE_LENGTH 512
#define PRODUCT_FIELDS 9
#define GRAMS_PER_KG 1000
#define CENTS_PER_EURO 100
#define VAN_MAX_WEIGHT_GRAMS (100 * GRAMS_PER_KG)
#define CSV_HEADER "lineCode,batchCode,batchDateTime,productId,productName,engineCode,binNumber,weightKg,priceEUR"

// This structure holds information about each product.
struct Product
{
    int lineCode;            // Shared by the products from one production line
    int batchCode;           // Shared by the products in one batch
    char batchDateTime[20];  // Batch production time: YYYY-MM-DD HH:MM
    int productId;           // Unique identifier for the product
    char productName[100];   // Name of the product
    char engineCode[20];     // Unique stock code for this product, as required by the report
    int binNumber;           // Unique storage bin for this product
    int weightGrams;        // Whole grams avoid floating-point comparison errors
    int priceCents;          // Whole cents keep the delivery price exact
};

struct Delivery
{
    int itemCount;
    int totalWeightGrams;
    long long totalPriceCents;
};

// Function prototypes for the four tasks
void mergeSort(struct Product arr[], struct Product temp[], int left, int right);
void merge(struct Product arr[], struct Product temp[], int left, int mid, int right);
int loadBatchFile(const char *filename, struct Product products[], int capacity, int *count);
int mergeBatchFiles(const char *files[], int batchCount, struct Product products[], int capacity);
int binarySearch(const struct Product arr[], int size, int targetWeightGrams);
struct Delivery calculateDelivery(const struct Product products[], int size, int maxWeightGrams);
void displayProducts(const struct Product products[], int size);

// Remove spaces from the beginning and end of a field.
static char *trim(char *text)
{
    while (isspace((unsigned char)*text)) text++;
    size_t length = strlen(text);
    while (length > 0 && isspace((unsigned char)text[length - 1])) text[--length] = '\0';
    return text;
}

// Read a complete line. Return 1 for success, 0 for EOF, or -1 for an error.
static int readLine(FILE *file, char line[], int capacity)
{
    if (fgets(line, capacity, file) == NULL) return ferror(file) ? -1 : 0;
    if (strchr(line, '\n') == NULL && !feof(file))
    {
        int ch = fgetc(file);
        if (ch != EOF)
        {
            while (ch != '\n' && ch != EOF) ch = fgetc(file);
            return -1;  // Discard an overlong line, including keyboard input
        }
    }
    return ferror(file) ? -1 : 1;
}

// Convert decimal text directly to whole units, without rounding.
// For example, "1.25" with 3 decimal places becomes 1250 grams.
static int parseUnits(const char *text, int decimalPlaces, int *value)
{
    int result = 0, digits = 0, fractionalDigits = 0, afterPoint = 0;
    while (*text != '\0')
    {
        if (*text == '.' && !afterPoint && decimalPlaces > 0)
        {
            afterPoint = 1;
        }
        else if (*text >= '0' && *text <= '9')
        {
            int digit = *text - '0';
            if (afterPoint && ++fractionalDigits > decimalPlaces) return 0;
            if (result > (INT_MAX - digit) / 10) return 0;
            result = result * 10 + digit;
            digits++;
        }
        else return 0;
        text++;
    }
    if (digits == 0 || (afterPoint && fractionalDigits == 0)) return 0;
    for (int i = fractionalDigits; i < decimalPlaces; i++)
    {
        if (result > INT_MAX / 10) return 0;
        result *= 10;
    }
    *value = result;
    return 1;
}

// Check the calendar date as well as the time format.
static int validDateTime(const char *text)
{
    const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int year, month, day, hour, minute;
    if (strlen(text) != 16 || text[4] != '-' || text[7] != '-' ||
        text[10] != ' ' || text[13] != ':') return 0;
    for (int i = 0; i < 16; i++)
    {
        if (i != 4 && i != 7 && i != 10 && i != 13 &&
            !isdigit((unsigned char)text[i])) return 0;
    }
    if (sscanf(text, "%4d-%2d-%2d %2d:%2d", &year, &month, &day, &hour, &minute) != 5)
        return 0;
    if (year < 1 || month < 1 || month > 12 || hour > 23 || minute > 59) return 0;
    int maxDay = daysInMonth[month - 1];
    if (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) maxDay++;
    return day >= 1 && day <= maxDay;
}

// These batch files use nine plain CSV fields, without quotes or embedded commas.
static int parseProduct(char line[], struct Product *product)
{
    char *fields[PRODUCT_FIELDS];
    char *next = line;
    for (int i = 0; i < PRODUCT_FIELDS; i++)
    {
        char *comma = strchr(next, ',');
        if ((i < PRODUCT_FIELDS - 1 && comma == NULL) ||
            (i == PRODUCT_FIELDS - 1 && comma != NULL)) return 0;
        if (comma != NULL) *comma = '\0';
        fields[i] = trim(next);
        if (*fields[i] == '\0' || strchr(fields[i], '"') != NULL) return 0;
        for (const char *p = fields[i]; *p != '\0'; p++)
            if (iscntrl((unsigned char)*p)) return 0;
        if (comma != NULL) next = comma + 1;
    }
    if (!parseUnits(fields[0], 0, &product->lineCode) || product->lineCode == 0 ||
        !parseUnits(fields[1], 0, &product->batchCode) || product->batchCode == 0 ||
        !parseUnits(fields[3], 0, &product->productId) || product->productId == 0 ||
        !parseUnits(fields[6], 0, &product->binNumber) || product->binNumber == 0 ||
        !parseUnits(fields[7], 3, &product->weightGrams) || product->weightGrams == 0 ||
        !parseUnits(fields[8], 2, &product->priceCents)) return 0;
    if (!validDateTime(fields[2]) || strlen(fields[4]) >= sizeof(product->productName) ||
        strlen(fields[5]) >= sizeof(product->engineCode)) return 0;
    strcpy(product->batchDateTime, fields[2]);
    strcpy(product->productName, fields[4]);
    strcpy(product->engineCode, fields[5]);
    return 1;
}

// Task 2: Read one separate production batch and add it to the dispatch list.
// On failure, restore the count so the caller cannot use an incomplete batch.
int loadBatchFile(const char *filename, struct Product products[], int capacity, int *count)
{
    FILE *file = fopen(filename, "r");
    char line[LINE_LENGTH];
    int start = *count, lineNumber = 1, status;
    const char *error = NULL;
    if (file == NULL)
    {
        fprintf(stderr, "Cannot open batch file '%s'. Run from the folder containing the CSV files.\n", filename);
        return 0;
    }
    if (readLine(file, line, sizeof(line)) != 1 || strcmp(trim(line), CSV_HEADER) != 0)
        error = "missing or incorrect CSV header";

    while (error == NULL && (status = readLine(file, line, sizeof(line))) != 0)
    {
        struct Product product = {0};
        lineNumber++;
        if (status < 0) { error = "line too long or file read error"; break; }
        if (*trim(line) == '\0') continue;
        if (*count >= capacity) { error = "dispatch list capacity exceeded"; break; }
        if (!parseProduct(line, &product)) { error = "invalid product record"; break; }
        if (*count > start && (product.lineCode != products[start].lineCode ||
            product.batchCode != products[start].batchCode ||
            strcmp(product.batchDateTime, products[start].batchDateTime) != 0))
        {
            error = "products in a batch must share its line, batch code and production time";
            break;
        }
        for (int i = 0; i < *count; i++)
        {
            if (product.productId == products[i].productId ||
                strcmp(product.engineCode, products[i].engineCode) == 0 ||
                product.binNumber == products[i].binNumber)
            {
                error = "duplicate product ID, engine code or bin number";
                break;
            }
            if (i < start && (product.lineCode == products[i].lineCode ||
                              product.batchCode == products[i].batchCode))
            {
                error = "separate batches must use distinct line and batch codes";
                break;
            }
        }
        if (error == NULL) products[(*count)++] = product;
    }
    if (fclose(file) != 0 && error == NULL) error = "file close error";
    if (error != NULL)
    {
        fprintf(stderr, "%s, line %d: %s.\n", filename, lineNumber, error);
        *count = start;
        return 0;
    }
    return 1;
}

// Task 2: Merge the four production batches into one dispatch list.
// Concatenate the files in order; Task 1 sorts the resulting list afterwards.
int mergeBatchFiles(const char *files[], int batchCount, struct Product products[], int capacity)
{
    int count = 0;
    for (int i = 0; i < batchCount; i++)
    {
        int start = count;
        if (!loadBatchFile(files[i], products, capacity, &count)) return -1;
        printf("Batch %d: %d products (%s)", i + 1, count - start, files[i]);
        if (count > start)
            printf(" - Line %d, batch %d, %s", products[start].lineCode,
                   products[start].batchCode, products[start].batchDateTime);
        printf("\n");
    }
    return count;
}

// Task 1: Sorting the products by weight using Merge Sort.
// This function uses divide-and-conquer and takes O(N log N) time.
void mergeSort(struct Product arr[], struct Product temp[], int left, int right)
{
    if (left < right)
    {
        int mid = left + (right - left) / 2;      // Find the middle index
        mergeSort(arr, temp, left, mid);         // Sort the left half
        mergeSort(arr, temp, mid + 1, right);    // Sort the right half
        merge(arr, temp, left, mid, right);      // Merge the sorted halves
    }
}

// Merge two sorted halves, using the same temporary array for every call.
void merge(struct Product arr[], struct Product temp[], int left, int mid, int right)
{
    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right)
    {
        if (arr[i].weightGrams <= arr[j].weightGrams)
            temp[k++] = arr[i++];  // Take the left product on ties to keep the sort stable
        else
            temp[k++] = arr[j++];
    }
    while (i <= mid) temp[k++] = arr[i++];
    while (j <= right) temp[k++] = arr[j++];
    for (i = left; i <= right; i++) arr[i] = temp[i];
}

// Task 3: Binary search for the first product with the requested weight.
// The list must be sorted. Whole grams allow exact comparisons in O(log N) time.
int binarySearch(const struct Product arr[], int size, int targetWeightGrams)
{
    int left = 0, right = size - 1, result = -1;
    while (left <= right)
    {
        int mid = left + (right - left) / 2;
        if (arr[mid].weightGrams == targetWeightGrams)
        {
            result = mid;
            right = mid - 1;  // Keep looking to the left for the first occurrence
        }
        else if (arr[mid].weightGrams < targetWeightGrams) left = mid + 1;
        else right = mid - 1;
    }
    return result;
}

// Read a positive weight in kg, with up to three decimal places (whole grams).
static int readSearchWeight(int *weightGrams)
{
    char line[LINE_LENGTH];
    while (1)
    {
        printf("Enter a weight in kg (up to 3 decimal places): ");
        fflush(stdout);
        int status = readLine(stdin, line, sizeof(line));
        if (status == 0 || ferror(stdin))
        {
            printf("\nNo input available; search skipped.\n");
            return 0;
        }
        if (status == 1 && parseUnits(trim(line), 3, weightGrams) && *weightGrams > 0) return 1;
        printf("Invalid weight. Enter a positive number such as 0.05 or 7.5.\n");
    }
}

// Task 4: Calculate the delivery summary with a maximum weight of 100 kg.
// The sorted list is loaded lightest first; stop when the next product cannot fit.
struct Delivery calculateDelivery(const struct Product products[], int size, int maxWeightGrams)
{
    struct Delivery delivery = {0, 0, 0};
    for (int i = 0; i < size; i++)
    {
        if (products[i].weightGrams > maxWeightGrams - delivery.totalWeightGrams) break;
        delivery.totalWeightGrams += products[i].weightGrams;
        delivery.totalPriceCents += products[i].priceCents;
        delivery.itemCount++;
    }
    return delivery;
}

// Display all product fields except the production time, shown in the batch summary.
void displayProducts(const struct Product products[], int size)
{
    printf("%-4s | %-24s | %9s | %10s | %-5s | %-5s | %-10s | %s\n",
           "ID", "Product Name", "Weight kg", "Price EUR", "Line", "Batch", "Engine", "Bin");
    printf("--------------------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < size; i++)
    {
        printf("%-4d | %-24s | %9.3f | %10.2f | %-5d | %-5d | %-10s | %d\n",
               products[i].productId, products[i].productName,
               products[i].weightGrams / (double)GRAMS_PER_KG,
               products[i].priceCents / (double)CENTS_PER_EURO,
               products[i].lineCode, products[i].batchCode,
               products[i].engineCode, products[i].binNumber);
    }
}

// Main function
int main(void)
{
    const char *batchFiles[] = {"batch1.csv", "batch2.csv", "batch3.csv", "batch4.csv"};
    int batchCount = (int)(sizeof(batchFiles) / sizeof(batchFiles[0]));
    struct Product *products = malloc(MAX_PRODUCTS * sizeof(*products));
    if (products == NULL)
    {
        fprintf(stderr, "Not enough memory for the dispatch list.\n");
        return EXIT_FAILURE;
    }

    // Task 2 runs before Task 1 because the separate batches must first be combined.
    printf("\nTASK 2 - MERGE BATCHES\nPRODUCTION BATCH SUMMARY\n");
    int count = mergeBatchFiles(batchFiles, batchCount, products, MAX_PRODUCTS);
    if (count < 0) { free(products); return EXIT_FAILURE; }
    printf("Total products merged: %d\n", count);

    // Task 1: Sort every product in the merged dispatch list.
    if (count > 1)
    {
        struct Product *temp = malloc((size_t)count * sizeof(*temp));
        if (temp == NULL)
        {
            fprintf(stderr, "Not enough memory for Merge Sort.\n");
            free(products);
            return EXIT_FAILURE;
        }
        mergeSort(products, temp, 0, count - 1);
        free(temp);
    }
    printf("\nTASK 1 - DISPATCH LIST SORTED BY WEIGHT\n");
    displayProducts(products, count);

    // Task 3: Search the sorted dispatch list by weight.
    int searchWeight;
    printf("\nTASK 3 - BINARY SEARCH\n");
    if (readSearchWeight(&searchWeight))
    {
        int index = binarySearch(products, count, searchWeight);
        if (index >= 0)
        {
            printf("Product found (first matching weight):\n");
            printf("Product ID: %d\nProduct Name: %s\nWeight: %.3f kg\nBatch: %d\nLine: %d\n",
                   products[index].productId, products[index].productName,
                   products[index].weightGrams / (double)GRAMS_PER_KG,
                   products[index].batchCode, products[index].lineCode);
        }
        else printf("Product with weight %.3f kg not found.\n", searchWeight / (double)GRAMS_PER_KG);
    }

    // Task 4: Show the selected products and delivery totals.
    struct Delivery delivery = calculateDelivery(products, count, VAN_MAX_WEIGHT_GRAMS);
    printf("\nTASK 4 - DELIVERY SUMMARY\n");
    printf("Maximum van weight: %.2f kg\n", VAN_MAX_WEIGHT_GRAMS / (double)GRAMS_PER_KG);
    printf("Products included (lightest first):\n");
    for (int i = 0; i < delivery.itemCount; i++)
        printf("  %2d. ID %-3d %-24s %.3f kg\n", i + 1, products[i].productId,
               products[i].productName, products[i].weightGrams / (double)GRAMS_PER_KG);
    printf("Items loaded: %d\n", delivery.itemCount);
    printf("Total weight of the delivery: %.3f kg\n", delivery.totalWeightGrams / (double)GRAMS_PER_KG);
    printf("Total price of the delivery: EUR %lld.%02lld\n",
           delivery.totalPriceCents / CENTS_PER_EURO, delivery.totalPriceCents % CENTS_PER_EURO);
    printf("Products left for another delivery: %d\n", count - delivery.itemCount);

    free(products);
    return EXIT_SUCCESS;
}
