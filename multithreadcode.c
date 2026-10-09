#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>

#define NUM_FILES 20
#define MAX_LINE 1000
#define INITIAL_CAPACITY 1000000
#define NUM_THREADS 16  // Reduce number of threads
#define BUFFER_SIZE 65536  // 64KB buffer

/* -------- Struct Definitions -------- */
struct SalesRecord {
    char region[50];
    char country[50];
    char itemType[50];
    char salesChannel[50];
    double revenue;
    double profit;
};

struct ThreadData {
    int start;
    int end;
    char filterType;
    char filterValue[128];
    int totalOrders;
    double totalRevenue;
    double totalProfit;
};

/* -------- Global Variables -------- */
struct SalesRecord *records = NULL;
int recordCount = 0;
int capacity = 0;

/* -------- Function Declarations -------- */
void readDataFromFiles();
void addRecord(struct SalesRecord r);
void freeRecords();
void showMenu();
void filterDataMT(char choice,  char *input);
void* threadFunc(void *arg);

/* -------- Add Record to Dynamic Array -------- */
void addRecord(struct SalesRecord x) {
    if (recordCount >= capacity) {
        capacity = (capacity == 0) ? INITIAL_CAPACITY : capacity * 2;
        records = realloc(records, capacity * sizeof(struct SalesRecord));
        if (!records) {
            printf("Memory allocation failed!\n");
            exit(1);
        }
    }
    records[recordCount++] = x;
}

/* -------- Main -------- */
int main() {
    clock_t start = clock();

    readDataFromFiles();
    showMenu();

    clock_t end = clock();
    double totalTime = (double)(end - start) / CLOCKS_PER_SEC;
    printf("\nExecution Time: %.4f seconds\n", totalTime);
  //

    freeRecords();
    return 0;
}

/* -------- Read CSV Files -------- */
void readDataFromFiles() {
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

   // printf("Loading data...\n");

    for (int i = 0; i < NUM_FILES; i++) {
        FILE *file = fopen(files[i], "r");
        if (!file) {
            printf("Cannot open file: %s\n", files[i]);
            continue;
        }

        // Use large buffer for faster reading
        char *buffer = malloc(BUFFER_SIZE);
        if (buffer) {
            setvbuf(file, buffer, _IOFBF, BUFFER_SIZE);
        }

        char line[MAX_LINE];
        fgets(line, sizeof(line), file); // skip header

        while (fgets(line, sizeof(line), file)) {
            struct SalesRecord r;
            char *token = strtok(line, ",");
            int col = 0;
            while (token) {
                switch (col) {
                    case 0: strncpy(r.region, token, sizeof(r.region)-1); r.region[sizeof(r.region)-1] = '\0'; break;
                    case 1: strncpy(r.country, token, sizeof(r.country)-1); r.country[sizeof(r.country)-1] = '\0'; break;
                    case 2: strncpy(r.itemType, token, sizeof(r.itemType)-1); r.itemType[sizeof(r.itemType)-1] = '\0'; break;
                    case 3: strncpy(r.salesChannel, token, sizeof(r.salesChannel)-1); r.salesChannel[sizeof(r.salesChannel)-1] = '\0'; break;
                    case 11: r.revenue = strtod(token, NULL); break;
                    case 13: r.profit  = strtod(token, NULL); break;
                }
                token = strtok(NULL, ",");
                col++;
            }
            addRecord(r);
        }
        fclose(file);
        if (buffer) free(buffer);

        //printf("Loaded file %d/%d\r", i+1, NUM_FILES);
        fflush(stdout);
    }
    printf("Strarting %d Thereds \n",NUM_THREADS);
    printf("All threds completed. Merging data.....\n");
    printf("Total records loaded: %d\n", recordCount);
}

/* -------- Menu -------- */
void showMenu() {
    char choice;
    char input[128];

    printf("\n.................< Welcome to my system >..............\n\n");
    printf("Select filter by:\n");
    printf("a. Region\nb. Country\nc. Item Type\nd. Sales Channel\n");
    printf("Enter choice (a/b/c/d)-> ");
    scanf(" %c", &choice);
    choice = tolower(choice);
    while (getchar() != '\n');

    printf("\nEnter the value to search: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';  // Remove newline only

    filterDataMT(choice, input);
}

/* -------- Thread Function -------- */
void* threadFunc(void *arg) {
    struct ThreadData *td = (struct ThreadData*)arg;
    td->totalOrders = 0;
    td->totalRevenue = 0;
    td->totalProfit = 0;

    for (int i = td->start; i < td->end; i++) {
        int match = 0;
        switch(td->filterType) {
            case 'a': match = (strcasecmp(records[i].region, td->filterValue) == 0); break;
            case 'b': match = (strcasecmp(records[i].country, td->filterValue) == 0); break;
            case 'c': match = (strcasecmp(records[i].itemType, td->filterValue) == 0); break;
            case 'd': match = (strcasecmp(records[i].salesChannel, td->filterValue) == 0); break;
        }
        if (match) {
            td->totalOrders++;
            td->totalRevenue += records[i].revenue;
            td->totalProfit  += records[i].profit;
        }
    }
    return NULL;
}

/* -------- Multi-threaded Filtering -------- */
void filterDataMT(char choice,  char *input) {
    pthread_t threads[NUM_THREADS];
    struct ThreadData tData[NUM_THREADS];

    int chunk = recordCount / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        tData[i].start = i * chunk;
        tData[i].end = (i == NUM_THREADS - 1) ? recordCount : tData[i].start + chunk;
        tData[i].filterType = choice;
        strcpy(tData[i].filterValue, input);
        pthread_create(&threads[i], NULL, threadFunc, &tData[i]);
    }

    long totalOrders = 0;
    double totalRevenue = 0, totalProfit = 0;

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        totalOrders += tData[i].totalOrders;
        totalRevenue += tData[i].totalRevenue;
        totalProfit += tData[i].totalProfit;
    }
printf("\n*****************************************");
    printf("\n...................... Filter Results ................\n");
    printf("Total Orders: %ld\n", totalOrders);
    printf("Total Revenue: %.2f\n", totalRevenue);
    printf("Total Profit: %.2f\n", totalProfit);
    printf("************************************\n");
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
