# ThingsBoard Dashboard Plan

Create devices:
- BAY_01
- BAY_02
- BAY_03

For each bay add:
1. Status card -> bayStatus
2. Gauge -> power
3. Gauge -> current
4. Gauge -> voltage
5. Temperature -> temperature
6. Time series -> voltage/current/power
7. Energy -> energyWh
8. Predicted arrival -> predictedArrivalProb
9. Predicted duration -> predictedDurationMin
10. Decision table -> loadDecision + throttleLevel

Station-level:
- Total station power = sum of bay powers
- Active alarms
- Allocation/decision history

RPC methods:
- setRelayState: true/false
- setThrottle: 0..100
- getStatus

MQTT telemetry topic:
v1/devices/me/telemetry
