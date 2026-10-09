#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define NUM_FILES 20
#define MAX_LINE 1000
#define INITIAL_CAPACITY 1000000

/* -------- Struct Definition -------- */
struct SalesRecord {
    char region[50];
    char country[50];
    char itemType[50];
    char salesChannel[50];
    double revenue;
    double profit;
};

/* -------- Global Variables -------- */
struct SalesRecord *records = NULL;  // Dynamic array
int recordCount = 0;                 // Current number of records
int capacity = 0;                    // Current capacity

/* -------- Function Declarations -------- */
void readDataFromFile();
void showMenu();
void filterData(char choice, const char *input);
void addRecord(struct SalesRecord x);
void freeRecords();

/* -------- Add Record to Array -------- */
void addRecord(struct SalesRecord x) {
    // If we reached max capacity, double the array size
    if (recordCount >= capacity) {
        capacity = (capacity == 0) ? INITIAL_CAPACITY : capacity * 2;
        records = realloc(records, capacity * sizeof(struct SalesRecord));
        if (!records) {
            printf("Memory allocation failed!\n");
            exit(1);
        }
    }

    // Copy record directly into the array
    records[recordCount++] = x;
}

/* -------- Main Function -------- */
int main() {
    clock_t start = clock();

    readDataFromFile();   // Read all data
    showMenu();           // Display menu and calculate

    clock_t end = clock();
    double totalTime = (double)(end - start) / CLOCKS_PER_SEC;
    printf("\nExecution Time: %.4f seconds\n", totalTime);
    printf("Total records loaded: %d\n", recordCount);

    freeRecords();
    return 0;
}

/* -------- Read all CSV files -------- */
void readDataFromFile() {
    char *files[NUM_FILES] = {
        "/media/sf_sales-data-1M/xaa.csv", "/media/sf_sales-data-1M/xab.csv",
        "/media/sf_sales-data-1M/xac.csv", "/media/sf_sales-data-1M/xad.csv",
        "/media/sf_sales-data-1M/xae.csv", "/media/sf_sales-data-1M/xaf.csv",
        "/media/sf_sales-data-1M/xag.csv", "/media/sf_sales-data-1M/xah.csv",
        "/media/sf_sales-data-1M/xai.csv", "/media/sf_sales-data-1M/xaj.csv",
        "/media/sf_sales-data-1M/xak.csv", "/media/sf_sales-data-1M/xal.csv",
        "/media/sf_sales-data-1M/xam.csv", "/media/sf_sales-data-1M/xan.csv",
        "/media/sf_sales-data-1M/xao.csv", "/media/sf_sales-data-1M/xap.csv",
        "/media/sf_sales-data-1M/xaq.csv", "/media/sf_sales-data-1M/xar.csv",
        "/media/sf_sales-data-1M/xas.csv", "/media/sf_sales-data-1M/xat.csv"
    };

    printf("Loading data...\n");

    for (int i = 0; i < NUM_FILES; i++) {
        FILE *file = fopen(files[i], "r");
        if (!file) {
            printf("Cannot open file: %s\n", files[i]);
            continue;
        }

        char line[MAX_LINE];
        // Skip header line
        if (!fgets(line, sizeof(line), file)) {
            fclose(file);
            continue;
        }

        // Read each line
        while (fgets(line, sizeof(line), file)) {
            struct SalesRecord x;
            char *token = strtok(line, ",");
            int col = 0;

            while (token) {
                switch (col) {
                    case 0:  strncpy(x.region, token, sizeof(x.region)-1);
                             x.region[sizeof(x.region)-1] = '\0'; break;
                    case 1:  strncpy(x.country, token, sizeof(x.country)-1);
                             x.country[sizeof(x.country)-1] = '\0'; break;
                    case 2:  strncpy(x.itemType, token, sizeof(x.itemType)-1);
                             x.itemType[sizeof(x.itemType)-1] = '\0'; break;
                    case 3:  strncpy(x.salesChannel, token, sizeof(x.salesChannel)-1);
                             x.salesChannel[sizeof(x.salesChannel)-1] = '\0'; break;
                    case 11: x.revenue = strtod(token, NULL); break;
                    case 13: x.profit  = strtod(token, NULL); break;
                }
                token = strtok(NULL, ",");
                col++;
            }

            addRecord(x);
        }
        fclose(file);
        printf("Loaded file %d/%d\r", i+1, NUM_FILES);
        fflush(stdout);
    }
    printf("\nData loading complete!\n");
}

/* -------- Menu -------- */
void showMenu() {
    char choice;
    char input[128];

    printf("\n.................< Welcome to my system >..............\n");
    printf("\nSelect filter by:\n");
    printf("a. Region\nb. Country\nc. Item Type\nd. Sales Channel\n");
    printf("Enter choice (a/b/c/d)-> ");
    scanf(" %c", &choice);
    choice = tolower(choice);
    while (getchar() != '\n'); // clear buffer

    printf("\nEnter the value to search: ");
    fgets(input, sizeof(input), stdin);
    int len = strlen(input);
    if (len > 0 && input[len-1] == '\n')
        input[len-1] = '\0';

    filterData(choice, input);
}

/* -------- Filtering Function (Much faster!) -------- */
void filterData(char choice, const char *input) {
    long totalOrders = 0;
    double totalRevenue = 0.0;
    double totalProfit = 0.0;

    printf("\nSearching through %d records...\n", recordCount);

    // Loop through the array once
    for (int i = 0; i < recordCount; i++) {
        int match = 0;

        switch(choice) {
            case 'a': match = (strcmp(records[i].region, input) == 0); break;
            case 'b': match = (strcmp(records[i].country, input) == 0); break;
            case 'c': match = (strcmp(records[i].itemType, input) == 0); break;
            case 'd': match = (strcmp(records[i].salesChannel, input) == 0); break;
        }

        if (match) {
            totalOrders++;
            totalRevenue += records[i].revenue;
            totalProfit  += records[i].profit;
        }
    }

    printf("\n******************************************************\n");
    printf("...........Filter Results.................\n");
    printf("Total number of orders: %ld\n", totalOrders);
    printf("Total revenue: %.2f\n", totalRevenue);
    printf("Total profit: %.2f\n", totalProfit);
    printf("******************************************************\n");
}

/* -------- Free Memory -------- */
void freeRecords() {
    if (records) {
        free(records);
        records = NULL;
    }
    recordCount = 0;
    capacity = 0;
}
