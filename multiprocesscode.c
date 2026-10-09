#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>    // for strcasecmp
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_FILES   20
#define MAX_LINE    1000
#define NUM_PROCESSES 20          // similar to NUM_THREADS
#define INITIAL_CAPACITY 1000000
#define BUFFER_SIZE 65536          // 64KB buffer

/* -------- Struct Definitions -------- */
struct SalesRecord {
    char region[50];
    char country[50];
    char itemType[50];
    char salesChannel[50];
    double revenue;
    double profit;
};

struct ProcResult {
    long totalOrders;
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
void filterDataMP(char choice, char *input);

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

    freeRecords();
    return 0;
}

/* -------- Read CSV Files (Naive part) -------- */
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
            memset(&r, 0, sizeof(r));

            char *token = strtok(line, ",");
            int col = 0;
            while (token) {
                switch (col) {
                    case 0:
                        strncpy(r.region, token, sizeof(r.region)-1);
                        r.region[sizeof(r.region)-1] = '\0';
                        break;
                    case 1:
                        strncpy(r.country, token, sizeof(r.country)-1);
                        r.country[sizeof(r.country)-1] = '\0';
                        break;
                    case 2:
                        strncpy(r.itemType, token, sizeof(r.itemType)-1);
                        r.itemType[sizeof(r.itemType)-1] = '\0';
                        break;
                    case 3:
                        strncpy(r.salesChannel, token, sizeof(r.salesChannel)-1);
                        r.salesChannel[sizeof(r.salesChannel)-1] = '\0';
                        break;
                    case 11:
                        r.revenue = strtod(token, NULL);
                        break;
                    case 13:
                        r.profit  = strtod(token, NULL);
                        break;
                }
                token = strtok(NULL, ",");
                col++;
            }
            addRecord(r);
        }

        fclose(file);
        if (buffer) free(buffer);
    }

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
    while (getchar() != '\n');   // Clear input buffer

    printf("\nEnter the value to search: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';  // Remove newline only

    filterDataMP(choice, input);
}

/* -------- Multi-process Filtering -------- */
void filterDataMP(char choice, char *input) {
    int numProcs = NUM_PROCESSES;
    if (recordCount < NUM_PROCESSES) {
        numProcs = recordCount > 0 ? recordCount : 1;
    }

    int pipes[numProcs][2];
    pid_t pids[numProcs];

    int chunk = recordCount / numProcs;

    for (int i = 0; i < numProcs; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            // Child
            close(pipes[i][0]); // close read end

            struct ProcResult pr;
            pr.totalOrders = 0;
            pr.totalRevenue = 0.0;
            pr.totalProfit  = 0.0;

            int start = i * chunk;
            int end   = (i == numProcs - 1) ? recordCount : start + chunk;

            for (int idx = start; idx < end; idx++) {
                int match = 0;
                switch (choice) {
                    case 'a':
                        match = (strcasecmp(records[idx].region, input) == 0);
                        break;
                    case 'b':
                        match = (strcasecmp(records[idx].country, input) == 0);
                        break;
                    case 'c':
                        match = (strcasecmp(records[idx].itemType, input) == 0);
                        break;
                    case 'd':
                        match = (strcasecmp(records[idx].salesChannel, input) == 0);
                        break;
                }
                if (match) {
                    pr.totalOrders++;
                    pr.totalRevenue += records[idx].revenue;
                    pr.totalProfit  += records[idx].profit;
                }
            }

            // Send result to parent
            if (write(pipes[i][1], &pr, sizeof(pr)) != sizeof(pr)) {
                perror("write");
            }
            close(pipes[i][1]);
            _exit(0);
        } else {
            // Parent
            pids[i] = pid;
            close(pipes[i][1]); // close write end in parent
        }
    }

    // Parent collects results
    struct ProcResult total;
    total.totalOrders = 0;
    total.totalRevenue = 0.0;
    total.totalProfit  = 0.0;

    for (int i = 0; i < numProcs; i++) {
        struct ProcResult pr;
        ssize_t n = read(pipes[i][0], &pr, sizeof(pr));
        if (n == sizeof(pr)) {
            total.totalOrders += pr.totalOrders;
            total.totalRevenue += pr.totalRevenue;
            total.totalProfit  += pr.totalProfit;
        }
        close(pipes[i][0]);
    }

    // Wait for children to finish
    for (int i = 0; i < numProcs; i++) {
        waitpid(pids[i], NULL, 0);
    }

    printf("\n******a***********************************\n");
    printf("................. Filter Results ................\n");
    printf("Total Orders : %ld\n", total.totalOrders);
    printf("Total Revenue: %.2f\n", total.totalRevenue);
    printf("Total Profit : %.2f\n", total.totalProfit);
    printf("*****************************************\n");
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
