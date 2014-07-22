/* Codebase macros - Change as you might need.
 * Yes, Rogel, you can gloat all you want. You win, this is cleaner, though not by a whole lot.
 */

#ifndef __IMC2CFG_H__
#define __IMC2CFG_H__

#if !defined(IMCCIRCLE) && !defined(IMCSTANDALONE)
#define CH_IMCDATA(ch)           ((ch)->pcdata->imcchardata)
#define CH_IMCLEVEL(ch)          ((ch)->level)
#define CH_IMCNAME(ch)           ((ch)->name)
#define CH_IMCSEX(ch)            ((ch)->sex)
#define CH_IMCTITLE(ch)          ((ch)->pcdata->title)
#endif

#if defined(IMCSMAUG) || defined(IMCCHRONICLES)
#define SMAUGSOCIAL
#define SOCIAL_DATA SOCIALTYPE
#define CH_IMCRANK(ch)           ((ch)->pcdata->rank)
#endif

#if defined(IMCROM)
#define first_descriptor descriptor_list
#define CH_IMCRANK(ch)           (title_table[(ch)->class][(ch)->level][(ch)->sex == SEX_FEMALE ? 1 : 0])
#endif

#if defined(IMCMERC)
#define first_descriptor descriptor_list
#define CH_IMCRANK(ch)           (title_table[(ch)->class][(ch)->level][(ch)->sex == SEX_FEMALE ? 1 : 0])
#endif

#if defined(IMCACK)
#define first_descriptor first_desc
#define CH_IMCRANK(ch)           (class_table[(ch)->class].who_name)
#endif

#if defined(IMCUENVY)
#define SMAUGSOCIAL
#define SOCIAL_DATA SOC_INDEX_DATA
SOC_INDEX_DATA *find_social( char *command );
#define first_descriptor descriptor_list
#define CH_IMCRANK(ch)           (title_table[(ch)->class][(ch)->level][(ch)->sex == SEX_FEMALE ? 1 : 0])
#endif

/* Blasted circle, always gotta do thing the hard way! */
#if defined(IMCCIRCLE)
#define SMAUGSOCIAL

   /*
    * This should be in an act.social.h, if it existed. Introducing
    * it in an IMC patch would be too intrusive. 
    */
struct social_messg
{
   int act_nr;
   int hide;
   int min_victim_position;
   char *char_no_arg;
   char *others_no_arg;
   char *char_found;
   char *others_found;
   char *vict_found;
   char *not_found;
   char *char_auto;
   char *others_auto;
};

   /*
    * UNCOMMENT if mud has Ascii Pfile code installed. 
    */
   /*
    * #include "diskio.h" 
    */

extern struct social_messg *soc_mess_list;
struct social_messg *find_social( const char *name );

typedef struct social_messg SOCIAL_DATA;
typedef struct char_data CHAR_DATA;
typedef struct descriptor_data DESCRIPTOR_DATA;

extern const char *class_abbrevs[];

const char *title_female( int chclass, int level );
const char *title_male( int chclass, int level );

#define first_descriptor         descriptor_list
#define URANGE(a, b, c)          ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define CH_IMCDATA(ch)           ((ch)->player_specials->imcchardata)
#define CH_IMCLEVEL(ch)          GET_LEVEL(ch)
#define CH_IMCNAME(ch)           GET_NAME(ch)
#define CH_IMCTITLE(ch)          GET_TITLE(ch)
#define CH_IMCRANK(ch)           (GET_SEX(ch) == SEX_FEMALE ? title_female(GET_CLASS(ch), GET_LEVEL(ch))	\
								: title_male(GET_CLASS(ch), GET_LEVEL(ch)))
#define CH_IMCSEX(ch)            GET_SEX(ch)
#endif

#endif

#if defined(IMCSTANDALONE)

typedef unsigned char bool;

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE 1
#endif

#define CH_IMCDATA(ch)           ((ch)->imcchardata)
#define CH_IMCLEVEL(ch)          ((ch)->level)
#define CH_IMCNAME(ch)           ((ch)->name)
#define CH_IMCSEX(ch)            ((ch)->sex)
#define CH_IMCTITLE(ch)          ( "User" )
#define CH_IMCRANK(ch)           ( "User" )

typedef enum
{
   SEX_NEUTRAL, SEX_MALE, SEX_FEMALE
} genders;

#define CON_PLAYING 1
#define LOWER(c)		((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))

typedef struct user_data CHAR_DATA;
typedef struct conn_data DESCRIPTOR_DATA;

struct user_data
{
   struct imcchar_data *imcchardata;
   char *name;
   int level;
   short sex;
};

struct conn_data
{
   DESCRIPTOR_DATA *next;
   DESCRIPTOR_DATA *prev;
   CHAR_DATA *original;
   CHAR_DATA *character;
   short connected;
};

DESCRIPTOR_DATA *first_descriptor;
DESCRIPTOR_DATA *last_descriptor;

#endif
