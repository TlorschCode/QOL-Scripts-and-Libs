mt19937_64 gen_rand64: 61.6009 ms, 6.16 ns/call, checksum=1398791585773173423
mt19937_64 gen_urandint(0,100): 50.62 ms, 5.06 ns/call, checksum=500094014
mt19937_64 gen_urandint(0,1_000_000_123): 50.20 ms, 5.02 ns/call, checksum=5000940690945197

std::rand() gen_rand64: 345.14 ms, 34.51 ns/call, checksum=128850078066084192
std::rand() gen_urandint(0,100): 328.35 ms, 32.84 ns/call, checksum=0
std::rand() gen_urandint(0,1_000_000_123): 323.82 ms, 32.38 ns/call, checksum=2499939

asm gen_rand64: 12.42 ms, 1.24 ns/call, checksum=11614717784401239576
asm gen_urandint(0,100): 31.09 ms, 3.11 ns/call, checksum=499998252
asm gen_urandint(0,1_000_000_123): 52.63 ms, 5.26 ns/call, checksum=4999860667884618