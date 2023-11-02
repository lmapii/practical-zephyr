
- [Goals](#goals)
- [Prerequisites](#prerequisites)
- [TODO:](#todo)
- [Summary](#summary)
- [Further reading](#further-reading)

## Goals

## Prerequisites

## TODO:

move away from freestanding to west workspace

reason "why west" and not just cmake

maybe little outlook on threads and final words

## Summary

Stuff becomes pretty wild depending on the MCU, but that's not Zephyr's fault.

E.g., building an APP for something like the nRF53840 is no longer a "simple switch", since all of a sudden it's a multi CPU SoC, using OpenAMP, requiring child images, partition manager (this is now vendor specific) ... but still, Zephyr as such still works and migrating no longer means you need to start from scratch. build, test, analysis, configuration system, all still the same.

Application is always only as simple as the system architecture.

## Further reading
