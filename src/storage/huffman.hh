/*!\file
 * File containing table for Huffman coding.
 * Each symbol is coded by code sym[symbol][0] in sym[symbol][1] bits
 */
huff_sym_t sym[256] = {
	{ 0, 2 },
	{ 1, 2 },
	{ 8, 4 },
	{ 9, 4 },
	{ 10, 4 },
	{ 11, 4 },
	{ 25, 5 },
	{ 24, 5 },
	{ 13, 4 },
	{ 236, 8 },
	{ 58, 6 },
	{ 965, 10 },
	{ 962, 10 },
	{ 237, 8 },
	{ 477, 9 },
	{ 964, 10 },
	{ 476, 9 },
	{ 239, 8 },
	{ 967, 10 },
	{ 975, 10 },
	{ 961, 10 },
	{ 1954, 11 },
	{ 969, 10 },
	{ 1953, 11 },
	{ 960, 10 },
	{ 963, 10 },
	{ 1940, 11 },
	{ 1956, 11 },
	{ 966, 10 },
	{ 3904, 12 },
	{ 3905, 12 },
	{ 3937, 12 },
	{ 1948, 11 },
	{ 1971, 11 },
	{ 1961, 11 },
	{ 1967, 11 },
	{ 973, 10 },
	{ 3936, 12 },
	{ 1957, 11 },
	{ 3941, 12 },
	{ 1937, 11 },
	{ 3944, 12 },
	{ 1955, 11 },
	{ 1969, 11 },
	{ 1945, 11 },
	{ 1960, 11 },
	{ 1959, 11 },
	{ 3945, 12 },
	{ 1941, 11 },
	{ 1973, 11 },
	{ 1944, 11 },
	{ 1977, 11 },
	{ 1949, 11 },
	{ 3956, 12 },
	{ 1963, 11 },
	{ 1964, 11 },
	{ 971, 10 },
	{ 1981, 11 },
	{ 1962, 11 },
	{ 3965, 12 },
	{ 1966, 11 },
	{ 3971, 12 },
	{ 1983, 11 },
	{ 3968, 12 },
	{ 3969, 12 },
	{ 3972, 12 },
	{ 3960, 12 },
	{ 3975, 12 },
	{ 3970, 12 },
	{ 3973, 12 },
	{ 3974, 12 },
	{ 3977, 12 },
	{ 3976, 12 },
	{ 3979, 12 },
	{ 3961, 12 },
	{ 3980, 12 },
	{ 3978, 12 },
	{ 3982, 12 },
	{ 3981, 12 },
	{ 3985, 12 },
	{ 3983, 12 },
	{ 7968, 13 },
	{ 7969, 13 },
	{ 3986, 12 },
	{ 3964, 12 },
	{ 3987, 12 },
	{ 3988, 12 },
	{ 3989, 12 },
	{ 3991, 12 },
	{ 7984, 13 },
	{ 7985, 13 },
	{ 3993, 12 },
	{ 3994, 12 },
	{ 3995, 12 },
	{ 3996, 12 },
	{ 3997, 12 },
	{ 1999, 11 },
	{ 8000, 13 },
	{ 8001, 13 },
	{ 4001, 12 },
	{ 4002, 12 },
	{ 4003, 12 },
	{ 4004, 12 },
	{ 1975, 11 },
	{ 3952, 12 },
	{ 4005, 12 },
	{ 1958, 11 },
	{ 3953, 12 },
	{ 3990, 12 },
	{ 8020, 13 },
	{ 1965, 11 },
	{ 16048, 14 },
	{ 4007, 12 },
	{ 8021, 13 },
	{ 8017, 13 },
	{ 4011, 12 },
	{ 3940, 12 },
	{ 3957, 12 },
	{ 4009, 12 },
	{ 4006, 12 },
	{ 1936, 11 },
	{ 1979, 11 },
	{ 1974, 11 },
	{ 8016, 13 },
	{ 16049, 14 },
	{ 8025, 13 },
	{ 4013, 12 },
	{ 4014, 12 },
	{ 4015, 12 },
	{ 16064, 14 },
	{ 16065, 14 },
	{ 8033, 13 },
	{ 4017, 12 },
	{ 8036, 13 },
	{ 8037, 13 },
	{ 4019, 12 },
	{ 16080, 14 },
	{ 16081, 14 },
	{ 8041, 13 },
	{ 4021, 12 },
	{ 4022, 12 },
	{ 4023, 12 },
	{ 16096, 14 },
	{ 16097, 14 },
	{ 8049, 13 },
	{ 4025, 12 },
	{ 8052, 13 },
	{ 8053, 13 },
	{ 4027, 12 },
	{ 8056, 13 },
	{ 8057, 13 },
	{ 4029, 12 },
	{ 4030, 12 },
	{ 4031, 12 },
	{ 16128, 14 },
	{ 16129, 14 },
	{ 8065, 13 },
	{ 8066, 13 },
	{ 8067, 13 },
	{ 8068, 13 },
	{ 8069, 13 },
	{ 4035, 12 },
	{ 16144, 14 },
	{ 16145, 14 },
	{ 8073, 13 },
	{ 4037, 12 },
	{ 4038, 12 },
	{ 4039, 12 },
	{ 16160, 14 },
	{ 16161, 14 },
	{ 8081, 13 },
	{ 4041, 12 },
	{ 8084, 13 },
	{ 8085, 13 },
	{ 4043, 12 },
	{ 8088, 13 },
	{ 8089, 13 },
	{ 4045, 12 },
	{ 4046, 12 },
	{ 4047, 12 },
	{ 16192, 14 },
	{ 16193, 14 },
	{ 8097, 13 },
	{ 4049, 12 },
	{ 8100, 13 },
	{ 8101, 13 },
	{ 4051, 12 },
	{ 16208, 14 },
	{ 16209, 14 },
	{ 8105, 13 },
	{ 4053, 12 },
	{ 4054, 12 },
	{ 4055, 12 },
	{ 16224, 14 },
	{ 16225, 14 },
	{ 8113, 13 },
	{ 4057, 12 },
	{ 8116, 13 },
	{ 8117, 13 },
	{ 4059, 12 },
	{ 8120, 13 },
	{ 8121, 13 },
	{ 4061, 12 },
	{ 4062, 12 },
	{ 4063, 12 },
	{ 16256, 14 },
	{ 16257, 14 },
	{ 8129, 13 },
	{ 8130, 13 },
	{ 8131, 13 },
	{ 8132, 13 },
	{ 8133, 13 },
	{ 4067, 12 },
	{ 16272, 14 },
	{ 16273, 14 },
	{ 8137, 13 },
	{ 4069, 12 },
	{ 4070, 12 },
	{ 4071, 12 },
	{ 16288, 14 },
	{ 16289, 14 },
	{ 8145, 13 },
	{ 4073, 12 },
	{ 8148, 13 },
	{ 8149, 13 },
	{ 4075, 12 },
	{ 8152, 13 },
	{ 8153, 13 },
	{ 4077, 12 },
	{ 4078, 12 },
	{ 4079, 12 },
	{ 16320, 14 },
	{ 16321, 14 },
	{ 8161, 13 },
	{ 4081, 12 },
	{ 8164, 13 },
	{ 8165, 13 },
	{ 4083, 12 },
	{ 16336, 14 },
	{ 16337, 14 },
	{ 8169, 13 },
	{ 4085, 12 },
	{ 4086, 12 },
	{ 4087, 12 },
	{ 16352, 14 },
	{ 16353, 14 },
	{ 8177, 13 },
	{ 4089, 12 },
	{ 4090, 12 },
	{ 4091, 12 },
	{ 8184, 13 },
	{ 8185, 13 },
	{ 4093, 12 },
	{ 4094, 12 },
	{ 4095, 12 },
	{ 28, 5 },
};