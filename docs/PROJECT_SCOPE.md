# Project Scope

The project is an Edge AI based Smart EV Charging Station Optimizer using ESP32, MQTT and ThingsBoard.

The intended architecture is:
Edge (ESP32) -> MQTT communication -> ThingsBoard cloud.

Each bay measures voltage/current/temperature, calculates power and energy, runs lightweight demand prediction locally, makes ALLOW/THROTTLE/DEFER decisions, and publishes telemetry. ThingsBoard provides dashboards, historical analytics, alarms, rule chains and RPC control.

The project is simulation-first. Real high-voltage EV charging hardware, payment integration and mobile app development are outside the defined scope.
