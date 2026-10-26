## Essentially, handling RAM

- When a required page is not found in ram, the OS has to load the required page from memory *into* the ram.
- If in case the ram is full, a *victim* page is selected to be slaughtered.
- Work has to be done to ensure the right page is selected for flushing.

- **Major algorithms in page replacement**
- First-in-First-out
- Optimal
- Least Recently used: kills the ones that are least used
- Least Frequentl used
- Not Recently Used
- Clock **(Used in major operating systems)**

Belady's anomaly.. I don't get it.
It was always about balancing *compute resource* with *accuracy*

- Jack the OOM killer.
