// Test the real program functions without maintaining a second implementation.
#define main sortingProgramMain
#include "../sorting.c"
#undef main
#include <assert.h>

static void checkProduct(const struct Product *actual, const struct Product *expected)
{
    assert(actual->lineCode == expected->lineCode);
    assert(actual->batchCode == expected->batchCode);
    assert(strcmp(actual->batchDateTime, expected->batchDateTime) == 0);
    assert(actual->productId == expected->productId);
    assert(strcmp(actual->productName, expected->productName) == 0);
    assert(strcmp(actual->engineCode, expected->engineCode) == 0);
    assert(actual->binNumber == expected->binNumber);
    assert(actual->weightGrams == expected->weightGrams);
    assert(actual->priceCents == expected->priceCents);
}

static void testSortAndSearch(void)
{
    const int sizes[] = {0, 1, 2, 3, 10, 40, MAX_PRODUCTS};
    struct Product *original = calloc(MAX_PRODUCTS, sizeof(*original));
    struct Product *products = calloc(MAX_PRODUCTS, sizeof(*products));
    struct Product *temp = calloc(MAX_PRODUCTS, sizeof(*temp));
    assert(original != NULL && products != NULL && temp != NULL);
    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++)
    {
        int count = sizes[s];
        for (int mode = 0; mode < 4; mode++)
        {
            for (int i = 0; i < count; i++)
            {
                struct Product p = {101, 1001, "2025-04-24 12:00", 0, "", "", 0, 0, 0};
                p.productId = i + 1;
                p.binNumber = i + 5;
                p.priceCents = 1234 + i;
                snprintf(p.productName, sizeof(p.productName), "Part %d", i);
                snprintf(p.engineCode, sizeof(p.engineCode), "ENG%d", i);
                if (mode == 0) p.weightGrams = i + 1;
                if (mode == 1) p.weightGrams = count - i;
                if (mode == 2) p.weightGrams = (i * 37 + 11) % 51 + 1;
                if (mode == 3) p.weightGrams = 17;
                original[i] = p;
                products[i] = p;
            }
            mergeSort(products, temp, 0, count - 1);
            for (int i = 0; i < count; i++)
            {
                int index = products[i].productId - 1;
                assert(index >= 0 && index < count);
                checkProduct(&products[i], &original[index]);
                if (i > 0)
                {
                    assert(products[i - 1].weightGrams <= products[i].weightGrams);
                    if (products[i - 1].weightGrams == products[i].weightGrams)
                        assert(products[i - 1].productId < products[i].productId);
                }
            }
            for (int target = 0; target <= MAX_PRODUCTS + 1; target++)
            {
                int expected = -1;
                // A separate linear oracle is used only in the test.
                for (int i = 0; i < count; i++)
                    if (products[i].weightGrams == target) { expected = i; break; }
                assert(binarySearch(products, count, target) == expected);
            }
        }
    }
    free(original);
    free(products);
    free(temp);
}

static void testDelivery(void)
{
    struct Product products[3] = {{0}};
    const int boundaries[] = {99999, 100000, 100001, INT_MAX};
    for (size_t i = 0; i < sizeof(boundaries) / sizeof(boundaries[0]); i++)
    {
        products[0].weightGrams = boundaries[i];
        products[0].priceCents = 12345;
        struct Delivery d = calculateDelivery(products, 1, VAN_MAX_WEIGHT_GRAMS);
        assert(d.itemCount == (boundaries[i] <= VAN_MAX_WEIGHT_GRAMS ? 1 : 0));
        assert(d.totalWeightGrams <= VAN_MAX_WEIGHT_GRAMS);
        assert(d.totalPriceCents == (d.itemCount ? 12345 : 0));
    }
    products[0].weightGrams = 100;
    products[1].weightGrams = 200;
    products[2].weightGrams = 99700;
    for (int i = 0; i < 3; i++) products[i].priceCents = INT_MAX;
    for (int size = 0; size <= 3; size++)
    {
        struct Delivery d = calculateDelivery(products, size, VAN_MAX_WEIGHT_GRAMS);
        assert(d.itemCount == size);
        assert(d.totalWeightGrams <= VAN_MAX_WEIGHT_GRAMS);
        assert(d.totalPriceCents == (long long)size * INT_MAX);
    }
    struct Delivery exact = calculateDelivery(products, 3, VAN_MAX_WEIGHT_GRAMS);
    assert(exact.totalWeightGrams == VAN_MAX_WEIGHT_GRAMS);
    products[2].weightGrams++;
    struct Delivery over = calculateDelivery(products, 3, VAN_MAX_WEIGHT_GRAMS);
    assert(over.itemCount == 2 && over.totalWeightGrams == 300);
    assert(calculateDelivery(products, 3, 0).itemCount == 0);
    assert(calculateDelivery(products, 0, VAN_MAX_WEIGHT_GRAMS).itemCount == 0);

    struct Product *many = calloc(MAX_PRODUCTS, sizeof(*many));
    assert(many != NULL);
    for (int i = 0; i < MAX_PRODUCTS; i++)
    {
        many[i].weightGrams = 100;
        many[i].priceCents = 10;
        struct Delivery d = calculateDelivery(many, i + 1, VAN_MAX_WEIGHT_GRAMS);
        assert(d.itemCount == i + 1);
        assert(d.totalWeightGrams == (i + 1) * 100);
        assert(d.totalWeightGrams <= VAN_MAX_WEIGHT_GRAMS);
        assert(d.totalPriceCents == (i + 1) * 10);
    }
    free(many);
}

static void testParsing(void)
{
    int value;
    assert(parseUnits("0.05", 3, &value) && value == 50);
    assert(parseUnits("0.050", 3, &value) && value == 50);
    assert(parseUnits(".05", 3, &value) && value == 50);
    assert(parseUnits("7.5", 3, &value) && value == 7500);
    assert(parseUnits("12.34", 2, &value) && value == 1234);
    assert(parseUnits("2147483.647", 3, &value) && value == INT_MAX);
    assert(!parseUnits("2147483.648", 3, &value));
    const char *bad[] = {"", ".", "-1", "+1", "nan", "inf", "1e2", "0x10", "1kg",
                         "1.2.3", "0.0501", "1.", "9999999999999999999999"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
        assert(!parseUnits(bad[i], 3, &value));
    assert(!parseUnits("1.1", 0, &value));
    assert(!parseUnits("1.001", 2, &value));
    assert(validDateTime("2024-02-29 23:59"));
    assert(!validDateTime("2025-02-29 12:00"));
    assert(!validDateTime("2025-04-31 12:00"));
    assert(!validDateTime("2025-01-01 24:00"));
    assert(!validDateTime("2025-01-01 12:60"));
    assert(!validDateTime("0000-01-01 12:00"));
    assert(!validDateTime("2025-1-01 12:00"));
}

static void testLoaderCapacity(void)
{
    struct Product products[11] = {{0}};
    int count = 0;
    products[9].productId = -123;
    assert(!loadBatchFile("batch1.csv", products, 9, &count));
    assert(count == 0 && products[9].productId == -123);
    assert(loadBatchFile("batch1.csv", products, 10, &count));
    assert(count == 10);
    assert(!loadBatchFile("batch2.csv", products, 10, &count));
    assert(count == 10);
    assert(!loadBatchFile("missing.csv", products, 11, &count));
    assert(count == 10);
}

int main(void)
{
    testSortAndSearch();
    testDelivery();
    testParsing();
    testLoaderCapacity();
    puts("PASS: C algorithm, stability, complete-record, boundary, parser and loader tests");
    return EXIT_SUCCESS;
}
