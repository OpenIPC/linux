FILTER_BASIC="-e cpu-cycles -e instructions -e cache-references -e cache-misses -e branch-instructions -e branch-misses -e stalled-cycles-frontend -e stalled-cycles-backend -e L1-dcache-loads -e L1-dcache-load-misses -e L1-dcache-stores -e L1-dcache-store-misses -e L1-icache-load-misses -e context-switches -e cpu-migrations -e page-faults -e dTLB-load-misses -e dTLB-store-misses -e iTLB-load-misses -e branch-loads -e branch-load-misses"
FILTER_EX="-e LLC-loads -e LLC-load-misses -e LLC-stores -e LLC-store-misses"
CMDLINE="usleep"

./perf stat $FILTER_BASIC $FILTER_EX $CMDLINE $1
