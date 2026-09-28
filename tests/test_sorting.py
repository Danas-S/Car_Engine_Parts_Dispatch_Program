"""Build and test in temporary folders; leave the supplied batches/executable untouched."""

import csv
from decimal import Decimal
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FILES = [f"batch{i}.csv" for i in range(1, 5)]
FLAGS = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow", "-Werror"]
COMPILER = os.environ.get("CC") or shutil.which("gcc")
if not COMPILER and Path("C:/MinGW/bin/gcc.exe").is_file():
    COMPILER = "C:/MinGW/bin/gcc.exe"
ENV = os.environ.copy()
if COMPILER:
    # The installed MinGW compiler needs its DLL folder on PATH.
    ENV["PATH"] = str(Path(COMPILER).resolve().parent) + os.pathsep + ENV.get("PATH", "")


def read_rows(path):
    with path.open(newline="", encoding="ascii") as file:
        return list(csv.DictReader(file))


def grams(row):
    return int(Decimal(row["weightKg"]) * 1000)


def cents(row):
    return int(Decimal(row["priceEUR"]) * 100)


class SortingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not COMPILER:
            raise RuntimeError("GCC not found. Set CC to your gcc executable.")
        cls.build = tempfile.TemporaryDirectory(prefix="sorting-build-")
        cls.addClassCleanup(cls.build.cleanup)
        cls.program = Path(cls.build.name) / "sorting-test.exe"
        cls.units = Path(cls.build.name) / "sorting-units.exe"
        for source, output in [(ROOT / "sorting.c", cls.program),
                               (ROOT / "tests/test_sorting.c", cls.units)]:
            result = subprocess.run([COMPILER, *FLAGS, str(source), "-o", str(output)],
                                    capture_output=True, text=True, env=ENV, timeout=60)
            if result.returncode or result.stdout or result.stderr:
                raise RuntimeError(f"Strict build failed: {result.stdout}{result.stderr}")

    def setUp(self):
        self.folder = tempfile.TemporaryDirectory(prefix="sorting-data-")
        self.addCleanup(self.folder.cleanup)
        self.cwd = Path(self.folder.name)
        for filename in FILES:
            shutil.copy2(ROOT / filename, self.cwd / filename)

    def run_program(self, input_text="0.05\n"):
        return subprocess.run([str(self.program)], input=input_text, text=True,
                              capture_output=True, cwd=self.cwd, env=ENV, timeout=10)

    def update_rows(self, filename, rows):
        with (self.cwd / filename).open("w", newline="", encoding="ascii") as file:
            writer = csv.DictWriter(file, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)

    def all_rows(self):
        return [row for name in FILES for row in read_rows(self.cwd / name)]

    def assert_good(self, result):
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertIn("TASK 4 - DELIVERY SUMMARY", result.stdout)

    def assert_bad(self, result, text):
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(text, result.stderr)
        self.assertNotIn("TASK 1 -", result.stdout)

    def test_01_supplied_batches_and_identifiers(self):
        names = ["Piston", "Crankshaft", "Cylinder Head", "Engine Block", "Camshaft",
                 "Timing Belt", "Timing Chain", "Oil Pump", "Oil Filter", "Fuel Pump",
                 "Spark Plug", "Alternator", "Starter Motor", "Water Pump", "Exhaust Manifold",
                 "Intake Manifold", "Air Filter", "Radiator", "AC Compressor", "Fuel Injector",
                 "Turbocharger", "Flywheel", "Cylinder Head Cover", "Vibration Damper",
                 "Water Pump Pulley", "Timing Pulley", "Intake Valve", "Exhaust Valve",
                 "Connecting Rod", "Valve Spring", "Oil Cooler", "Intercooler", "EGR Valve",
                 "Throttle Body", "MAP Sensor", "Knock Sensor", "Oxygen Sensor",
                 "Fuel Pressure Regulator", "Fuel Pump Relay", "Ignition Coil"]
        for i, filename in enumerate(FILES):
            rows = read_rows(self.cwd / filename)
            self.assertEqual(len(rows), 10)
            self.assertEqual([r["productName"] for r in rows], names[i * 10:(i + 1) * 10])
            self.assertEqual({r["lineCode"] for r in rows}, {str((i + 1) * 100 + 1)})
            self.assertEqual({r["batchCode"] for r in rows}, {str((i + 1) * 1000 + 1)})
            self.assertEqual({r["batchDateTime"] for r in rows}, {f"2025-04-24 {12+i}:00"})
        rows = self.all_rows()
        for field in ("productId", "engineCode", "binNumber"):
            self.assertEqual(len({r[field] for r in rows}), 40)
        self.assertTrue(all(grams(r) > 0 and cents(r) >= 0 for r in rows))

    def test_02_merge_counts(self):
        result = self.run_program()
        self.assert_good(result)
        for i in range(1, 5):
            self.assertIn(f"Batch {i}: 10 products", result.stdout)
        self.assertIn("Total products merged: 40", result.stdout)
        stages = [result.stdout.index(f"TASK {i} -") for i in (2, 1, 3, 4)]
        self.assertEqual(stages, sorted(stages))

    def test_03_sorted_table_preserves_records_and_ties(self):
        result = self.run_program()
        self.assert_good(result)
        table = result.stdout.split("TASK 1 -", 1)[1].split("TASK 3 -", 1)[0]
        actual = [[cell.strip() for cell in line.split("|")]
                  for line in table.splitlines() if re.match(r"^\d+\s*\|", line)]
        expected = sorted(self.all_rows(), key=grams)  # Python's independent stable sort
        self.assertEqual(len(actual), len(expected))
        for got, row in zip(actual, expected):
            self.assertEqual(got[:2], [row["productId"], row["productName"]])
            self.assertEqual(Decimal(got[2]), Decimal(row["weightKg"]))
            self.assertEqual(Decimal(got[3]), Decimal(row["priceEUR"]))
            self.assertEqual(got[4:], [row[k] for k in ("lineCode", "batchCode", "engineCode", "binNumber")])
        weights = [Decimal(row[2]) for row in actual]
        self.assertTrue(all(a <= b for a, b in zip(weights, weights[1:])))

    def test_04_known_weight(self):
        result = self.run_program("35\n")
        self.assert_good(result)
        self.assertIn("Product ID: 4\nProduct Name: Engine Block", result.stdout)
        self.assertIn("Batch: 1001\nLine: 101", result.stdout)

    def test_05_unknown_weights(self):
        for weight in ("0.001", "7.5", "999"):
            with self.subTest(weight=weight):
                result = self.run_program(weight + "\n")
                self.assert_good(result)
                self.assertIn("not found.", result.stdout)

    def test_06_first_duplicate_and_equivalent_decimal_input(self):
        for weight in ("0.05", "0.050", ".05", " 0.05  "):
            with self.subTest(weight=weight):
                result = self.run_program(weight + "\n")
                self.assert_good(result)
                self.assertIn("Product ID: 11\nProduct Name: Spark Plug", result.stdout)

    def test_07_delivery_products_and_totals(self):
        result = self.run_program()
        self.assert_good(result)
        included, weight, price = [], 0, 0
        for row in sorted(self.all_rows(), key=grams):
            if weight + grams(row) > 100000:
                break
            included.append(row["productId"])
            weight += grams(row)
            price += cents(row)
            self.assertLessEqual(weight, 100000)
        self.assertEqual((len(included), weight, price), (39, 91860, 422000))
        summary = result.stdout.split("TASK 4 -", 1)[1]
        self.assertEqual(re.findall(r"\d+\. ID\s+(\d+)", summary), included)
        self.assertIn("Items loaded: 39", summary)
        self.assertIn("Total weight of the delivery: 91.860 kg", summary)
        self.assertIn("Total price of the delivery: EUR 4220.00", summary)
        self.assertNotIn("$", result.stdout)

    def test_08_invalid_input_recovers(self):
        invalid = ["abc", "", "nan", "inf", "-1", "0", "1kg", "0.0501", "1e2", "1.2.3",
                   "1.", "99999999999999999999", "x" * 2000]
        result = self.run_program("\n".join([*invalid, "0.05"]) + "\n")
        self.assert_good(result)
        self.assertEqual(result.stdout.count("Invalid weight."), len(invalid))
        self.assertIn("Product ID: 11", result.stdout)

    def test_09_eof_and_final_line_without_newline(self):
        for entry in ("", "bad", "0.05"):
            with self.subTest(entry=entry):
                result = self.run_program(entry)
                self.assert_good(result)
                if entry == "0.05":
                    self.assertIn("Product ID: 11", result.stdout)
                else:
                    self.assertIn("search skipped", result.stdout)

    def test_10_c_algorithm_and_boundary_tests(self):
        result = subprocess.run([str(self.units)], capture_output=True, text=True,
                                cwd=self.cwd, env=ENV, timeout=20)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("PASS:", result.stdout)

    def test_11_missing_file(self):
        (self.cwd / "batch3.csv").unlink()
        self.assert_bad(self.run_program(), "Cannot open batch file 'batch3.csv'")

    def test_12_malformed_records(self):
        original = read_rows(self.cwd / "batch1.csv")
        changes = [("productId", "0"), ("productId", "999999999999999999999"),
                   ("weightKg", "-1"), ("weightKg", "nan"), ("weightKg", "0"),
                   ("weightKg", "0.0001"), ("priceEUR", "-1"), ("priceEUR", "1.001"),
                   ("lineCode", ""), ("binNumber", "0"), ("batchDateTime", "2025-02-29 12:00"),
                   ("productName", "x" * 100), ("engineCode", "x" * 20),
                   ("productName", 'Part "one"'), ("productName", "Part,one")]
        for field, value in changes:
            with self.subTest(field=field, value=value):
                rows = [r.copy() for r in original]
                rows[0][field] = value
                self.update_rows("batch1.csv", rows)
                self.assert_bad(self.run_program(), "batch1.csv, line 2: invalid product record")

    def test_13_incorrect_header_and_field_counts(self):
        path = self.cwd / "batch1.csv"
        original = path.read_text()
        header, record, *_ = original.splitlines()
        for text in ("", "wrong\n" + record, header + "\n" + record + ",extra\n",
                     header + "\n" + record.rsplit(",", 1)[0], header + "\n" + "x" * 1000):
            with self.subTest(text=text[:40]):
                path.write_text(text, encoding="ascii")
                self.assert_bad(self.run_program(), "batch1.csv")

    def test_14_duplicate_ids_engines_and_bins(self):
        original = read_rows(self.cwd / "batch2.csv")
        first = read_rows(self.cwd / "batch1.csv")[0]
        for field in ("productId", "engineCode", "binNumber"):
            with self.subTest(field=field):
                rows = [r.copy() for r in original]
                rows[0][field] = first[field]
                self.update_rows("batch2.csv", rows)
                self.assert_bad(self.run_program(), "duplicate product ID, engine code or bin number")

    def test_15_batch_and_line_semantics(self):
        original = read_rows(self.cwd / "batch2.csv")
        for field, value in (("lineCode", "101"), ("batchCode", "1001")):
            with self.subTest(field=field):
                rows = [r.copy() for r in original]
                rows[0][field] = value
                self.update_rows("batch2.csv", rows)
                self.assert_bad(self.run_program(), "distinct line and batch codes")
        for field, value in (("lineCode", "999"), ("batchCode", "9999"),
                             ("batchDateTime", "2025-04-24 14:00")):
            with self.subTest(field=field):
                rows = [r.copy() for r in original]
                rows[1][field] = value
                self.update_rows("batch2.csv", rows)
                self.assert_bad(self.run_program(), "share its line, batch code and production time")

    def test_16_changed_record_count(self):
        rows = read_rows(self.cwd / "batch4.csv")
        extra = rows[-1].copy()
        extra.update(productId="41", engineCode="ENG0041", binNumber="41", productName="Extra Filter")
        self.update_rows("batch4.csv", rows + [extra])
        result = self.run_program()
        self.assert_good(result)
        self.assertIn("Batch 4: 11 products", result.stdout)
        self.assertIn("Total products merged: 41", result.stdout)

    def test_17_empty_batches_and_empty_dispatch(self):
        for filename in FILES:
            path = self.cwd / filename
            path.write_text(path.read_text().splitlines()[0] + "\n", encoding="ascii")
        result = self.run_program()
        self.assert_good(result)
        self.assertIn("Total products merged: 0", result.stdout)
        self.assertIn("not found.", result.stdout)
        self.assertIn("Items loaded: 0", result.stdout)
        self.assertIn("EUR 0.00", result.stdout)

    def test_18_capacity_limit(self):
        first = read_rows(self.cwd / "batch1.csv")[0]
        rows = []
        for i in range(1001):
            row = first.copy()
            row.update(productId=str(i + 1), engineCode=f"TEST{i+1}", binNumber=str(i + 1))
            rows.append(row)
        self.update_rows("batch1.csv", rows)
        self.assert_bad(self.run_program(), "dispatch list capacity exceeded")

    def test_19_crlf_lf_blank_lines_and_no_final_newline(self):
        for filename in FILES:
            path = self.cwd / filename
            lines = path.read_text().splitlines()
            path.write_text("\n\n".join(lines), encoding="ascii")
        self.assert_good(self.run_program())


if __name__ == "__main__":
    unittest.main(verbosity=2)
