# Implementation Checklist

## Milestone 1 — One bay
- [ ] Open Wokwi
- [ ] ESP32 + 2 potentiometers + DHT22 + 2 buttons + relay + 3 LEDs
- [ ] Run firmware
- [ ] Verify voltage/current/temperature in Serial Monitor
- [ ] Press plug-in: FREE -> CHARGING
- [ ] Press plug-out: CHARGING -> FREE
- [ ] Verify relay and LEDs

## Milestone 2 — ThingsBoard
- [ ] Create BAY_01
- [ ] Copy access token
- [ ] Put token into firmware
- [ ] Run Wokwi
- [ ] Confirm telemetry arrives
- [ ] Build dashboard

## Milestone 3 — AI
- [ ] Run generate_data.py
- [ ] Run train_models.py
- [ ] Record accuracy and R2
- [ ] Integrate model.h
- [ ] Confirm predictedArrivalProb and predictedDurationMin

## Milestone 4 — Optimization
- [ ] Test overcurrent -> THROTTLE
- [ ] Test high load -> THROTTLE
- [ ] Test free bay + low prediction -> DEFER
- [ ] Otherwise -> ALLOW

## Milestone 5 — Multi-bay
- [ ] Duplicate device for BAY_02
- [ ] Duplicate device for BAY_03
- [ ] Aggregate station power
- [ ] Test peak-load scenario

## Milestone 6 — Final demo
- [ ] Normal-load scenario
- [ ] Peak-load scenario
- [ ] Sensor/connection fault scenario
- [ ] RPC manual override
- [ ] Capture dashboard screenshots
- [ ] Compare unmanaged vs optimized peak load
