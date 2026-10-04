# Rule Chain Plan

1. Overcurrent:
   current > overloadCurrentA -> Critical alarm

2. Offline:
   no telemetry for >60 s -> Major/device offline alarm

3. High demand:
   predictedArrivalProb > predictionThreshold AND all bays charging
   -> Warning alarm

4. Aggregation:
   on telemetry -> calculate totalStationPower

5. Tariff update:
   scheduled update -> shared attributes:
   maxStationLoadW, overloadCurrentA, predictionThreshold,
   peakTariffStart, peakTariffEnd
