import numpy as np
import pandas as pd

rng = np.random.default_rng(42)
N = 8000

hour = rng.integers(0, 24, N)
day = rng.integers(0, 7, N)
bay = rng.integers(0, 2, N)
recent = np.clip(rng.normal(12, 5, N), 0, 32)
elapsed = np.where(bay == 1, rng.uniform(0, 120, N), 0)
neighbor = rng.integers(0, 3, N)

morning = np.exp(-0.5 * ((hour - 9) / 2.0) ** 2)
evening = np.exp(-0.5 * ((hour - 19) / 2.5) ** 2)
weekday = ((day >= 1) & (day <= 5)).astype(float)

logit = -2.2 + 3.0*morning + 3.2*evening + 0.15*weekday + 0.15*neighbor
p = 1/(1+np.exp(-logit))
arrived = rng.binomial(1, p)

actual_duration = 55 + 0.35*elapsed - 0.8*recent + rng.normal(0, 8, N)
actual_duration = np.clip(actual_duration, 10, 180)

df = pd.DataFrame({
    "hourOfDay": hour,
    "dayOfWeek": day,
    "bayOccupied": bay,
    "recentAvgCurrent": recent,
    "sessionElapsedMin": elapsed,
    "neighborBaysOccupied": neighbor,
    "arrivedWithinWindow": arrived,
    "actualDurationMin": actual_duration
})

df.to_csv("synthetic_ev_data.csv", index=False)
print("Created synthetic_ev_data.csv with", len(df), "rows")
