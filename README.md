# C sorting project

The program reads four separate production batches, combines them, sorts the
dispatch list by weight, searches it and calculates a delivery of at most 100 kg.
The supplied dataset contains four batches of ten products. The program uses the
actual loaded count, including when records are added or removed.

## Build and run

Open PowerShell in this assignment folder. The installed compiler is at
`C:\MinGW\bin\gcc.exe`; add its folder to PATH for the current shell:

```powershell
$env:PATH = 'C:\MinGW\bin;' + $env:PATH
gcc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror sorting.c -o sorting.exe
.\sorting.exe
```

On another machine with GCC already on PATH, the same compiler command works.
Run from the folder containing `batch1.csv` through `batch4.csv`. These files
must accompany the executable. No external library is required.

Enter, for example, `0.05` to find the first 50 g product, or `7.5` for a weight
absent from the sample data. Invalid input is rejected and the prompt repeats.
EOF skips the search and still prints the delivery summary.

## How the tasks work

1. **Task 2 - merge batches:** `mergeBatchFiles()` calls `loadBatchFile()` for
   each CSV file. Every record is validated and appended to the dispatch array.
   It reports each batch's count and returns the actual total. A failed file
   stops the program before sorting or delivering an incomplete dispatch list.
2. **Task 1 - Merge Sort:** `mergeSort()` divides the loaded range, sorts both
   halves and calls `merge()`. Whole Product records move together. On equal
   weights, the left product is selected first, preserving file/row order.
   Sorting takes O(N log N) time and O(N) temporary storage, allocated once.
3. **Task 3 - Binary Search:** the sorted list is halved repeatedly. After a
   match, the search continues left to find the first equal weight. The search
   is O(log N). Input uses `fgets()` and checked decimal parsing, not unchecked
   `scanf()`.
4. **Task 4 - delivery:** consider products from lightest to heaviest. Add a
   product only if its weight fits in the remaining capacity; otherwise stop.
   Later products cannot fit either because the list is sorted. List the
   selected products, item count, exact weight and total price in EUR.

Task 2 executes before Task 1, while the original assignment numbering is kept.
The simple duplicate checks during file loading take O(N squared); this does
not change the Merge Sort or Binary Search algorithm complexities above.

## Exact units and file format

The Product structure retains the original identifying fields. Two fields are
renamed to make their units explicit: `weightGrams` and `priceCents` are integers.
CSV files and console input still use kg and EUR. `parseUnits()` converts decimal
digits directly: `0.05` and `0.050` both become 50 grams. This removes floating
point equality and accumulation errors without introducing a search tolerance.
Console weights show three decimal places to represent the supported gram
resolution; the sample data remains approximate, not measured to the gram.

The delivery uses an integer limit of 100000 grams, with no tolerance that could
permit an overweight load. Prices accumulate in a `long long` number of cents.
Floating point is used only for formatting individual weights and prices, never
for sorting, searching or capacity decisions. The final total price is formatted
directly from whole euros and the remaining cents.

Each CSV starts with this exact header:

```text
lineCode,batchCode,batchDateTime,productId,productName,engineCode,binNumber,weightKg,priceEUR
```

- Nine plain comma-separated fields, without quoted fields or embedded commas.
- Positive integer line, batch, product and bin codes.
- Date/time in `YYYY-MM-DD HH:MM` format, with calendar validation.
- One shared line, batch code and production time per file; distinct lines and
  batch codes between nonempty files.
- Globally unique product IDs, engine codes and bin numbers.
- Positive kg weights, with up to three decimal places; nonnegative EUR prices,
  with up to two decimal places. Use a decimal point. Exponent notation, signs,
  NaN/infinity, trailing text and excess decimal places are rejected.
- Names up to 99 characters, engine codes up to 19 characters, and lines up to
  510 characters excluding the newline. Blank lines are ignored. LF and CRLF
  are accepted; files can omit the final newline. Save as plain ASCII or UTF-8
  without a byte-order mark.

`MAX_PRODUCTS` is an explicit safety capacity of 1000, not the expected count.
The supplied forty records use only forty entries. Adding a product requires no
algorithm changes; exceeding the capacity produces an error rather than writing
past the array. Header-only batches and an empty dispatch list are handled.
Both allocations are checked and freed, and every opened batch file is closed.

## Data and expected result

See [DATA_NOTES.md](DATA_NOTES.md) for all forty before/after weights and prices,
the batch/code audit, part assumptions and manufacturer references. The original
ENG404 duplicate is removed. All four batches contain ten products, with unique
IDs, stock engine codes and bins.

The supplied data produces **39 items, 91.86 kg, EUR 4220.00**. The 35 kg Engine
Block (ID 4) remains for another delivery. The method favours lighter items; it
does not optimise the sale value or try combinations to fill every remaining kg.

## Tests

```powershell
python tests/test_sorting.py
```

Python's standard library is sufficient. Set the `CC` environment variable to a
GCC executable path if it is not on PATH or at `C:\MinGW\bin\gcc.exe`.
The runner compiles both the actual program and a C test harness using the strict
flags above. It works in temporary directories, preserving the real batch files
and `sorting.exe`.

Verified result: **19 automated tests passed**, with no compiler warnings.

| Requested check | Result |
|---|---|
| Four batches, ten expected products each | PASS |
| All batches merged, forty total | PASS |
| Every adjacent sorted weight ascending | PASS |
| Known weight found | PASS |
| Absent weight not found | PASS |
| Stable ties and first matching weight | PASS |
| Delivery always at or below 100 kg | PASS |
| Invalid keyboard input recovers | PASS |
| Unique IDs and engine codes | PASS |
| Strict warning-enabled builds | PASS |

Additional checks cover complete-record preservation, sorted/reverse/repeated
weights, arrays of 0 to 1000 products, 99.999/100.000/100.001 kg boundaries,
repeated decimal additions, large price totals, EOF, long input, missing files,
malformed headers/rows, invalid dates, duplicate bins, inconsistent batches,
capacity overflow, forty-one products, empty files with headers and line endings.

## Word documents to update later

The three Word documents have deliberately been left unchanged.

**projectreportsorting.docx**

- **Test Data:** replace `>10 products per line` with `10 products per line`,
  retain four lines and forty products, and update sample values/codes/dates.
  Explain that engine codes are unique demonstration stock codes.
- **Data Structure Design:** describe `weightGrams`, `priceCents` and the small
  `Delivery` summary structure; kg and EUR remain the external units.
- **Task 2: Merge Batches:** replace the statement that all forty hardcoded
  products are merged into one array with actual CSV loading, validation,
  separate batch counts and the returned total.
- **Task 1:** explain the single reusable temporary array instead of variable
  length temporary arrays in each merge. Stability and O(N log N) still apply.
- **Task 3:** explain validated line input, decimal-to-gram conversion and the
  first exact gram match, instead of comparing floats directly.
- **Task 4:** state lightest-first selection, whole-gram capacity checks, the
  included-product list and EUR totals. Replace examples with the new totals.
- **Flowchart:** the existing merge-then-sort order is still correct. Add file
  open/validation failures, real counts and checked input to any detailed flow.
- **Pseudocode, C Code and Full Working C Code:** replace old snippets and the
  embedded full source, particularly the giant initializer, missing file merge,
  hardcoded 39/40, float equality, unchecked input, dollar sign and obsolete
  `countProducts()` task comment.
- **Testing Plan:** record the executed tests and exact boundary results rather
  than relying on the old generic claims.
- **Conclusion:** file input is now implemented, so remove it from the list of
  future-only extensions.

**pseudocode.docx**

- **Task 1:** pass a reusable temporary array through `mergeSort` and `merge`,
  and compare `weightGrams` while copying complete records. Keep left-first ties.
- **Task 2:** keep the existing read-and-append design, but add CSV headers,
  record validation, bounds/open/read/close errors, actual per-batch counts and
  a returned total/error. Read records while the read succeeds, not by checking
  EOF before a read.
- **Task 3:** search `weightGrams` using the parsed integer target. Equality is
  valid for these integers. Keep the leftward search after a match; add invalid
  input retry and EOF handling outside the search function.
- **Task 4:** use grams/cents, test remaining capacity before addition, return a
  summary and print the included prefix of the sorted list plus EUR totals.
- Add the overall execution order: load/merge (Task 2), sort (Task 1), search
  (Task 3), delivery (Task 4), with the actual count passed throughout.

**sorting_ccode.docx** is also an old source snapshot; replace its contents with
the final `sorting.c` when you decide to refresh the documentation.
