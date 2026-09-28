# Demonstration data audit

All 40 original part names and product IDs are retained. Each row represents **one
part**, not a set of pistons, valves, injectors or coils. Weights are rounded
illustrative estimates for small/medium passenger-car parts, without packaging or
fluids. The parts do not belong to one specific vehicle. Prices are illustrative
EUR amounts for ordinary aftermarket/replacement parts, not current quotations.
No VAT, labour or shipping calculation is part of this assignment.

The estimates were reviewed individually. In particular, the block is a bare
block, the cylinder head is a bare head, the camshaft is one shaft, the intake
manifold can be a light alloy/plastic assembly, the fuel pump is a small electric
pump, the injector is a petrol injector, and the flywheel is a conventional
single-mass flywheel. These choices avoid comparing an individual small part
with the weight of a complete assembly or kit.

## Codes, dates and batches

| File | Product IDs / bins | Shared line | Shared batch | Shared production time |
|---|---|---:|---:|---|
| batch1.csv | 1-10 | 101 | 1001 | 2025-04-24 12:00 |
| batch2.csv | 11-20 | 201 | 2001 | 2025-04-24 13:00 |
| batch3.csv | 21-30 | 301 | 3001 | 2025-04-24 14:00 |
| batch4.csv | 31-40 | 401 | 4001 | 2025-04-24 15:00 |

The original code used a different line and batch code for almost every product.
Each batch now shares its line, batch code and batch-level production timestamp.
The original day is retained. Bins 1-40 remain unique warehouse positions.
Engine codes are synthetic unique stock identifiers ENG0001-ENG0040, following
the report's uniqueness requirement; they are not vehicle engine-family codes.
This removes the original ENG404 collision between Timing Chain (ID 7) and
Throttle Body (ID 34). The loader rejects duplicate IDs, engine codes and bins.

## Research used to check the scale of the weights

These manufacturer examples are reference points, not exact specifications for
the generic records. Other weights and every EUR price remain explicitly
labelled demonstration estimates. Sources were consulted on 28 September 2026.

- Bosch lists manifold pressure/temperature sensors around 22-24 g. The generic
  MAP sensor is therefore represented as 0.03 kg. [Bosch sensor range](https://www.bosch-motorsport.com/products/sensors/pressure-and-temperature/)
- Bosch lists knock sensors of 48, 60 and 82 g; the generic knock sensor uses
  0.05 kg. [Bosch knock sensors](https://www.bosch-motorsport.com/products/sensors/knock/)
- Bosch's HDEV 6 petrol injector has versions of 58 g and 63-102 g without wire;
  the dataset uses 0.10 kg. [Bosch HDEV 6](https://www.bosch-motorsport.com/content/downloads/Raceparts/en-GB/418288395420303755.html)
- Bosch's P65-WS coil weighs less than 222 g without wire. The generic coil's
  0.25 kg is an approximate allowance for a different housing/connection design,
  not a claimed P65-WS measurement. [Bosch P65-WS](https://www.bosch-motorsport.com/products/fuel-and-spark/ignition-coils/ignition-coil-p65-ws/)
- Bosch's 2019 catalogue lists electronic throttle bodies around 0.9-1.1 kg;
  the generic throttle body uses 0.90 kg. [Bosch catalogue, printed page 198](https://www.bosch-motorsport.com/media/downloads/catalogs/catalog_2019.pdf)

## Every weight and price reviewed

Unchanged values were retained where reasonable; there is no need to alter a
plausible value merely to make it different. Large changes correct the scale of
single valves, sensors, springs, coils, pulleys and the engine block.

| ID | Product | Old kg | New kg | Old EUR | New EUR |
|---|---|---:|---:|---:|---:|
| 1 | Piston | 2.5 | 0.50 | 50.0 | 60.00 |
| 2 | Crankshaft | 8.0 | 15.00 | 200.0 | 350.00 |
| 3 | Cylinder Head | 12.0 | 12.00 | 300.0 | 450.00 |
| 4 | Engine Block | 15.0 | 35.00 | 400.0 | 700.00 |
| 5 | Camshaft | 4.0 | 3.00 | 80.0 | 120.00 |
| 6 | Timing Belt | 1.0 | 0.15 | 25.0 | 25.00 |
| 7 | Timing Chain | 2.0 | 0.50 | 45.0 | 60.00 |
| 8 | Oil Pump | 3.0 | 1.50 | 60.0 | 90.00 |
| 9 | Oil Filter | 0.5 | 0.30 | 15.0 | 15.00 |
| 10 | Fuel Pump | 4.0 | 0.70 | 90.0 | 90.00 |
| 11 | Spark Plug | 0.3 | 0.05 | 8.0 | 8.00 |
| 12 | Alternator | 6.0 | 6.00 | 150.0 | 180.00 |
| 13 | Starter Motor | 5.5 | 3.50 | 120.0 | 120.00 |
| 14 | Water Pump | 4.2 | 1.50 | 70.0 | 70.00 |
| 15 | Exhaust Manifold | 8.5 | 5.00 | 190.0 | 190.00 |
| 16 | Intake Manifold | 5.0 | 2.50 | 110.0 | 150.00 |
| 17 | Air Filter | 0.4 | 0.30 | 10.0 | 15.00 |
| 18 | Radiator | 6.5 | 5.00 | 160.0 | 160.00 |
| 19 | AC Compressor | 3.0 | 6.00 | 100.0 | 250.00 |
| 20 | Fuel Injector | 0.6 | 0.10 | 22.0 | 65.00 |
| 21 | Turbocharger | 10.0 | 7.00 | 250.0 | 450.00 |
| 22 | Flywheel | 7.5 | 8.00 | 180.0 | 180.00 |
| 23 | Cylinder Head Cover | 4.5 | 1.50 | 75.0 | 75.00 |
| 24 | Vibration Damper | 9.2 | 2.50 | 140.0 | 100.00 |
| 25 | Water Pump Pulley | 1.7 | 0.40 | 30.0 | 30.00 |
| 26 | Timing Pulley | 2.3 | 0.50 | 35.0 | 35.00 |
| 27 | Intake Valve | 0.8 | 0.05 | 12.0 | 12.00 |
| 28 | Exhaust Valve | 1.1 | 0.05 | 18.0 | 18.00 |
| 29 | Connecting Rod | 3.6 | 0.60 | 65.0 | 65.00 |
| 30 | Valve Spring | 0.9 | 0.05 | 10.0 | 10.00 |
| 31 | Oil Cooler | 5.2 | 1.00 | 105.0 | 105.00 |
| 32 | Intercooler | 10.5 | 4.00 | 215.0 | 215.00 |
| 33 | EGR Valve | 2.7 | 1.00 | 55.0 | 100.00 |
| 34 | Throttle Body | 3.2 | 0.90 | 72.0 | 120.00 |
| 35 | MAP Sensor | 0.4 | 0.03 | 14.0 | 35.00 |
| 36 | Knock Sensor | 0.2 | 0.05 | 9.0 | 25.00 |
| 37 | Oxygen Sensor | 0.1 | 0.15 | 11.0 | 60.00 |
| 38 | Fuel Pressure Regulator | 1.5 | 0.20 | 40.0 | 40.00 |
| 39 | Fuel Pump Relay | 0.6 | 0.03 | 19.0 | 19.00 |
| 40 | Ignition Coil | 2.4 | 0.25 | 58.0 | 58.00 |

## Expected delivery

All records together weigh 126.86 kg and cost EUR 4920.00. Stable ascending
weight order loads 39 products totalling 91.86 kg and EUR 4220.00. The remaining
35 kg engine block would bring the load to 126.86 kg, so it is excluded.
This is a lightest-first selection, not a search for maximum revenue or the
closest possible total to 100 kg. Equal-weight products keep their original
file order and row order. For example, searching 0.05 kg returns the Spark Plug
(ID 11) before the later valves, spring and knock sensor of the same weight.
