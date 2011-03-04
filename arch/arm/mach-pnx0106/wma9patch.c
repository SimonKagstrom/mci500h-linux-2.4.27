
const unsigned long xmem_patch[] = {
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00000000,
0x00000001,
0x00000000,
0x00000000,
0x00000001,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000001,
0x00000001,
0x00000000,
0x00000040,
0x00000000,
0x00000000,
0x00003fab,
0x00005263,
0x00005265,
0x0000526d,
0x00005273,
0x0000527e,
0x0000528a,
/*
0x000057ab,
0x00006a63,
0x00006a65,
0x00006a6d,
0x00006a73,
0x00006a7e,
0x00006a8a,
*/
0x00ffffff,
0x007f8000,
0x00000000,
0x00804005,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00800011,
0x00800012,
0x00800013,
0x00800014,
0x00800015
};
// END XMEM INIT

const unsigned long ymem_patch[] = {
0x000007ff,
0x000007ff,
0x000007fe,
0x000007fe,
0x00000009,
0x000007c4,
0x000007fc,
0x00000027,
0x00000622,
0x000007fa,
0x00000059,
0x000001ba,
0x000007f8,
0x0000009e,
0x00000130,
0x000007f6,
0x000000f6,
0x00000328,
0x000007f4,
0x00000161,
0x00000650,
0x000007f2,
0x000001e0,
0x0000015a,
0x000007f0,
0x00000271,
0x000002f6,
0x000007ee,
0x00000315,
0x000001e0,
0x000007ec,
0x000003cb,
0x000004d2,
0x000007ea,
0x00000494,
0x0000028c,
0x000007e8,
0x0000056f,
0x000001d4,
0x000007e6,
0x0000065c,
0x00000172,
0x000007e4,
0x0000075b,
0x0000002e,
0x000007e3,
0x0000006b,
0x000004da,
0x000007e1,
0x0000018d,
0x0000064a,
0x000007df,
0x000002c1,
0x00000354,
0x000007dd,
0x00000406,
0x000002d0,
0x000007db,
0x0000055c,
0x0000039e,
0x000007d9,
0x000006c3,
0x0000049e,
0x000007d8,
0x0000003b,
0x000004b4,
0x000007d6,
0x000001c4,
0x000002ca,
0x000007d4,
0x0000035d,
0x000005c8,
0x000007d2,
0x00000507,
0x0000049e,
0x000007d0,
0x000006c1,
0x0000063c,
0x000007cf,
0x0000008c,
0x00000198,
0x000007cd,
0x00000266,
0x000005aa,
0x000007cb,
0x00000451,
0x0000016a,
0x000007c9,
0x0000064b,
0x000003d8,
0x000007c8,
0x00000055,
0x000003f4,
0x000007c6,
0x0000026f,
0x000000c2,
0x000007c4,
0x00000498,
0x00000146,
0x000007c2,
0x000006d0,
0x0000048c,
0x000007c1,
0x00000118,
0x0000019e,
0x000007bf,
0x0000036e,
0x0000078a,
0x000007bd,
0x000005d4,
0x00000564,
0x000007bc,
0x00000049,
0x0000023e,
0x000007ba,
0x000002cc,
0x0000052e,
0x000007b8,
0x0000055e,
0x0000054e,
0x000007b6,
0x000007ff,
0x000001ba,
0x000007b5,
0x000002ae,
0x00000190,
0x000007b3,
0x0000056b,
0x000003f2,
0x000007b2,
0x00000037,
0x00000000,
0x000007b0,
0x00000310,
0x000004e0,
0x000007ae,
0x000005f8,
0x000001bc,
0x000007ad,
0x000000ed,
0x000005be,
0x000007ab,
0x000003f1,
0x00000010,
0x000007a9,
0x00000701,
0x000007e2,
0x000007a8,
0x00000220,
0x00000466,
0x000007a6,
0x0000054c,
0x000004ce,
0x000007a5,
0x00000086,
0x00000050,
0x000007a3,
0x000003cc,
0x00000622,
0x000007a1,
0x00000720,
0x00000580,
0x000007a0,
0x00000281,
0x000005a4,
0x0000079e,
0x000005ef,
0x000005cc,
0x0000079d,
0x0000016a,
0x00000536,
0x0000079b,
0x000004f2,
0x00000328,
0x0000079a,
0x00000086,
0x000006e2,
0x00000798,
0x00000427,
0x000007aa,
0x00000796,
0x000007d5,
0x000004ca,
0x00000795,
0x0000038f,
0x0000058a,
0x00000793,
0x00000756,
0x00000134,
0x00000792,
0x00000328,
0x00000716,
0x00000790,
0x00000707,
0x00000682,
0x0000078f,
0x000002f2,
0x000006c4,
0x0000078d,
0x000006e9,
0x00000732,
0x0000078c,
0x000002ec,
0x0000071e,
0x0000078a,
0x000006fb,
0x000005de,
0x00000789,
0x00000316,
0x000002ce,
0x00000787,
0x0000073c,
0x00000542,
0x00000786,
0x0000036e,
0x00000498,
0x00000784,
0x000007ac,
0x0000002c,
0x00000783,
0x000003f4,
0x0000075c,
0x00000782,
0x00000049,
0x00000188,
0x00000780,
0x000004a8,
0x00000612,
0x0000077f,
0x00000113,
0x0000045e,
0x0000077d,
0x00000589,
0x000003ce,
0x0000077c,
0x0000020a,
0x000003ca,
0x0000077a,
0x00000696,
0x000003b8,
0x00000779,
0x0000032d,
0x00000304,
0x00000777,
0x000007cf,
0x00000114,
0x00000776,
0x0000047b,
0x00000558,
0x00000775,
0x00000132,
0x0000073e,
0x00000773,
0x000005f4,
0x00000630,
0x00000772,
0x000002c1,
0x000001a2,
0x00000770,
0x00000798,
0x00000104,
0x0000076f,
0x00000479,
0x000003ca,
0x0000076e,
0x00000165,
0x00000166,
0x0000076c,
0x0000065b,
0x00000150,
0x0000076b,
0x0000035b,
0x000002fe,
0x0000076a,
0x00000065,
0x000005e8,
0x00000768,
0x0000057a,
0x00000188,
0x00000767,
0x00000298,
0x00000558,
0x00000765,
0x000007c1,
0x000000d4,
0x00000764,
0x000004f3,
0x00000378,
0x00000763,
0x0000022f,
0x000004c4,
0x00000761,
0x00000775,
0x00000438,
0x00000760,
0x000004c5,
0x00000152,
0x0000075f,
0x0000021e,
0x00000396,
0x0000075d,
0x00000781,
0x00000288,
0x0000075c,
0x000004ed,
0x000005a8,
0x0000075b,
0x00000263,
0x00000480,
0x00000759,
0x000007e2,
0x00000694,
0x00000758,
0x0000056b,
0x0000036c,
0x00000757,
0x000002fd,
0x00000290,
0x00000756,
0x00000098,
0x0000038a,
0x00000754,
0x0000063c,
0x000005e6,
0x00000753,
0x000003ea,
0x0000012e,
0x00000752,
0x000001a0,
0x000004f0,
0x00000750,
0x00000760,
0x000000b8,
0x0000074f,
0x00000528,
0x00000416,
0x0000074e,
0x000002f9,
0x0000069a,
0x0000074d,
0x000000d3,
0x000007d6,
0x0000074b,
0x000006b6,
0x0000075a,
0x0000074a,
0x000004a2,
0x000004b8,
0x00000749,
0x00000296,
0x00000788,
0x00000748,
0x00000093,
0x00000758,
0x00000746,
0x00000699,
0x000003c4,
0x00000745,
0x000004a7,
0x00000460,
0x00000744,
0x000002be,
0x000000c2,
0x00000743,
0x000000dd,
0x00000084,
0x00000741,
0x00000704,
0x0000033e,
0x00000740,
0x00000534,
0x0000008c,
0x0000073f,
0x0000036c,
0x00000008,
0x0000073e,
0x000001ac,
0x0000014c,
0x0000073c,
0x000007f4,
0x000003f8,
0x0000073b,
0x00000644,
0x000007a8,
0x0000073a,
0x0000049d,
0x000003fc,
0x00000739,
0x000002fe,
0x00000090,
0x00000738,
0x00000166,
0x00000506,
0x00000736,
0x000007d7,
0x000000fe,
0x00000735,
0x0000064f,
0x0000041a,
0x00000734,
0x000004cf,
0x000005fc,
0x00000733,
0x00000357,
0x0000064a,
0x00000732,
0x000001e7,
0x000004a4,
0x00000731,
0x0000007f,
0x000000b0,
0x0000072f,
0x0000071e,
0x00000214,
0x0000072e,
0x000005c5,
0x00000078,
0x0000072d,
0x00000473,
0x00000380,
0x0000072c,
0x00000329,
0x000002d4,
0x0000072b,
0x000001e6,
0x0000061e,
0x0000072a,
0x000000ab,
0x00000506,
0x00000728,
0x00000777,
0x00000738,
0x00000727,
0x0000064b,
0x0000045a,
0x00000726,
0x00000526,
0x0000041c,
0x00000725,
0x00000408,
0x00000628,
0x00000724,
0x000002f2,
0x0000022a,
0x00000723,
0x000001e2,
0x000007d0,
0x00000722,
0x000000da,
0x000006c8,
0x00000720,
0x000007d9,
0x000006c0,
0x0000071f,
0x000006df,
0x00000768,
0x0000071e,
0x000005ed,
0x00000070,
0x0000071d,
0x00000501,
0x00000188,
0x0000071c,
0x0000041c,
0x00000260,
0x0000071b,
0x0000033e,
0x000002ac,
0x0000071a,
0x00000267,
0x0000021e,
0x00000719,
0x00000197,
0x00000068,
0x00000718,
0x000000cd,
0x0000053e,
0x00000717,
0x0000000b,
0x00000054,
0x00000715,
0x0000074f,
0x00000160,
0x00000714,
0x0000069a,
0x00000016,
0x00000713,
0x000005eb,
0x0000042e,
0x00000712,
0x00000543,
0x0000055c,
0x00000711,
0x000004a2,
0x00000358,
0x00000710,
0x00000407,
0x000005da,
0x0000070f,
0x00000373,
0x0000049a,
0x0000070e,
0x000002e5,
0x00000750,
0x0000070d,
0x0000025e,
0x000005b8,
0x0000070c,
0x000001dd,
0x0000078a,
0x0000070b,
0x00000163,
0x00000480,
0x0000070a,
0x000000ef,
0x00000456,
0x00000709,
0x00000081,
0x000006c6,
0x00000708,
0x0000001a,
0x0000038e,
0x00000706,
0x000007b9,
0x0000026a,
0x00000705,
0x0000075e,
0x00000316,
0x00000704,
0x00000709,
0x00000550,
0x00000703,
0x000006bb,
0x000000d6,
0x00000702,
0x00000672,
0x00000566,
0x00000701,
0x00000630,
0x000002c2,
0x00000700,
0x000005f4,
0x000000a4,
0x000006ff,
0x000005bd,
0x000006d2,
0x000006fe,
0x0000058d,
0x00000508,
0x000006fd,
0x00000563,
0x00000308,
0x000006fc,
0x0000053f,
0x00000094,
0x000006fb,
0x00000520,
0x0000056e,
0x000006fa,
0x00000508,
0x00000158,
0x000006f9,
0x000004f5,
0x00000416,
0x000006f8,
0x000004e8,
0x00000568,
0x000006f7,
0x000004e1,
0x00000514,
0x000006f6,
0x000004e0,
0x000002de,
0x000006f5,
0x000004e4,
0x0000068c,
0x000006f4,
0x000004ee,
0x000007de,
0x000006f3,
0x000004fe,
0x0000069e,
0x000006f2,
0x00000514,
0x00000290,
0x000006f1,
0x0000052f,
0x0000037c,
0x000006f0,
0x00000550,
0x00000126,
0x000006ef,
0x00000576,
0x00000356,
0x000006ee,
0x000005a2,
0x000001d4,
0x000006ed,
0x000005d3,
0x00000468,
0x000006ec,
0x0000060a,
0x000002da,
0x000006eb,
0x00000646,
0x000004f2,
0x000006ea,
0x00000688,
0x0000027a,
0x000006e9,
0x000006cf,
0x0000033c,
0x000006e8,
0x0000071b,
0x00000700,
0x000006e7,
0x0000076d,
0x00000590,
0x000006e6,
0x000007c4,
0x000006b8,
0x000006e6,
0x00000021,
0x00000244,
0x000006e5,
0x00000082,
0x000007fe,
0x000006e4,
0x000000e9,
0x000007b0,
0x000006e3,
0x00000156,
0x0000012a,
0x000006e2,
0x000001c7,
0x00000434,
0x000006e1,
0x0000023e,
0x0000009e,
0x000006e0,
0x000002b9,
0x00000636,
0x000006df,
0x0000033a,
0x000004c6,
0x000006de,
0x000003c0,
0x0000041c,
0x000006dd,
0x0000044b,
0x0000040a,
0x000006dc,
0x000004db,
0x0000045c,
0x000006db,
0x00000570,
0x000004e0,
0x000006da,
0x0000060a,
0x00000566,
0x000006d9,
0x000006a9,
0x000005c0,
0x000006d8,
0x0000074d,
0x000005ba,
0x000006d7,
0x000007f6,
0x00000528,
0x000006d7,
0x000000a4,
0x000003d6,
0x000006d6,
0x00000157,
0x00000198,
0x000006d5,
0x0000020e,
0x00000640,
0x000006d4,
0x000002cb,
0x0000019e,
0x000006d3,
0x0000038c,
0x00000382,
0x000006d2,
0x00000452,
0x000003c0,
0x000006d1,
0x0000051d,
0x0000022c,
0x000006d0,
0x000005ec,
0x00000696,
0x000006cf,
0x000006c1,
0x000000d2,
0x000006ce,
0x0000079a,
0x000000b4,
0x000006ce,
0x00000077,
0x00000610,
0x000006cd,
0x0000015a,
0x000000b8,
0x000006cc,
0x00000241,
0x00000080,
0x000006cb,
0x0000032c,
0x0000053e,
0x000006ca,
0x0000041c,
0x000006c8,
0x000006c9,
0x00000511,
0x000004f0,
0x000006c8,
0x0000060a,
0x0000078e,
0x000006c7,
0x00000708,
0x00000676,
0x000006c7,
0x0000000b,
0x0000017e,
0x000006c6,
0x00000112,
0x0000007e,
0x000006c5,
0x0000021d,
0x0000034a,
0x000006c4,
0x0000032d,
0x000001b8,
0x000006c3,
0x00000441,
0x000003a4,
0x000006c2,
0x0000055a,
0x000000e0,
0x000006c1,
0x00000677,
0x00000146,
0x000006c0,
0x00000798,
0x000004ae,
0x000006c0,
0x000000be,
0x000002ee,
0x000006bf,
0x000001e8,
0x000003e2,
0x000006be,
0x00000316,
0x0000075e,
0x000006bd,
0x00000449,
0x0000053e,
0x000006bc,
0x00000580,
0x0000055a,
0x000006bb,
0x000006bb,
0x0000078c,
0x000006ba,
0x000007fb,
0x000003ac,
0x000006ba,
0x0000013f,
0x00000194
};
// END YMEM INIT

const unsigned long xmem_patch_start = 0x2803;
const unsigned long ymem_patch_start = (0x6600 + 875);
const unsigned long xmem_patch_size = sizeof(xmem_patch);
const unsigned long ymem_patch_size = sizeof(ymem_patch);