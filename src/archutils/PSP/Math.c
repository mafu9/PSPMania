float cosf( float x )
{
	float result;
	__asm__ volatile (
		"lv.s		S000, %1\n"
		"vcst.s		S001, VFPU_2_PI\n"
		"vmul.s		S000, S000, S001\n"
		"vcos.s		S000, S000\n"
		"sv.s		S000, %0\n"
		: "=m"(result)
		: "m"(x)
	);
	return result;
}

float fmodf( float x, float y )
{
	float result;
	__asm__ volatile (
		"lv.s		S000, %1\n"
		"lv.s		S001, %2\n"
		"vrcp.s		S002, S001\n"
		"vmul.s		S003, S000, S002\n"
		"vf2iz.s	S002, S003, 0\n"
		"vi2f.s		S003, S002, 0\n"
		"vmul.s		S003, S003, S001\n"
		"vsub.s		S000, S000, S003\n"
		"sv.s		S000, %0\n"
		: "=m"(result)
		: "m"(x), "m"(y)
	);
	return result;
}

long lrintf( float x )
{
	long result;
	__asm__ volatile (
		"lv.s		S000, %1\n"
		"vf2in.s	S000, S000, 0\n"
		"sv.s		S000, %0\n"
		: "=m"(result)
		: "m"(x)
	);
	return result;
}

float powf( float x, float y )
{
	float result;
	__asm__ volatile (
		"lv.s		S000, %1\n"
		"lv.s		S001, %2\n"
		"vlog2.s	S000, S000\n"
		"vmul.s		S000, S000, S001\n"
		"vexp2.s	S000, S000\n"
		"sv.s		S000, %0\n"
		: "=m"(result)
		: "m"(x), "m"(y)
	);
	return result;
}

float sinf( float x )
{
	float result;
	__asm__ volatile (
		"lv.s		S000, %1\n"
		"vcst.s		S001, VFPU_2_PI\n"
		"vmul.s		S000, S000, S001\n"
		"vsin.s		S000, S000\n"
		"sv.s		S000, %0\n"
		: "=m"(result)
		: "m"(x)
	);
	return result;
}

void sincosf( float x, float *ps, float *pc )
{
	__asm__ volatile (
		"lv.s		S000, %2\n"
		"vcst.s		S001, VFPU_2_PI\n"
		"vmul.s		S000, S000, S001\n"
		"vrot.p		C002, S000, [s,c]\n"
		"sv.s		S002, %0\n"
		"sv.s		S003, %1\n"
		: "=m"(*ps), "=m"(*pc)
		: "m"(x)
	);
}