#ifndef CF_CHOREO_SHARED
#define CF_CHOREO_SHARED

typedef enum
{
	WP_WJAT = 0,
	WP_RIGHTHAND,
	WP_LEFTHAND,
	WP_RIGHTLEG,
	WP_LEFTLEG,
	WP_RIGHTBACK,
	WP_LEFTBACK,
} weaponposition_t;

typedef enum
{
	PDT_STOPEMISSION = -1,
	PDT_UNDEFINED = 0,
	PDT_ATTACHMENT,
	PDT_ORIGIN,
} particledispatch_t;

#endif