/* IMC2 Freedom Client - Developed by Mud Domain.
 *
 * Copyright ©2004 by Roger Libiez ( Samson )
 * Contributions by Johnathan Walker ( Xorith ), Copyright ©2004
 * Additional contributions by Jesse Defer ( Garil ), Copyright ©2004
 * Additional contributions by Rogel, Copyright ©2004
 * Comments and suggestions welcome: http://www.mudbytes.net/index.php?a=forum&f=31
 * License terms are available in the imc2freedom.license file.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>
#include <time.h>
#ifdef WIN32
#include <io.h>
#undef EINTR
#undef EMFILE
#define EINTR WSAEINTR
#define EMFILE WSAEMFILE
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#define MAXHOSTNAMELEN 32
#else
#include <fnmatch.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#if defined(__OpenBSD__) || defined(__FreeBSD__)
#include <sys/types.h>
#endif
#ifdef IMCSTANDALONE
   #include "imc.h"
#endif
#include "sha256.h"

#if defined(IMCCIRCLE)
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "imc.h"
#endif

#if defined(IMCSMAUG) || defined(IMCCHRONICLES)
#include "mud.h"
#ifdef WEBSVR
#include "web.h"
#endif
#if defined(IMCCHRONICLES)
#include "factions.h"
#include "files.h"
#endif
#endif
#if defined(IMCROM) || defined(IMCMERC) || defined(IMCUENVY) || defined(IMC1STMUD)
#include "merc.h"
#if defined(IMCROM)
#include "tables.h"
#endif
#endif
#if defined(IMCACK)
#include "globals.h"
#endif

#define IMCKEY( literal, field, value ) \
if( !strcasecmp( (word), (literal) ) )  \
{                                       \
   (field) = (value);                   \
   fMatch = TRUE;                       \
   break;                               \
}

int imcwait;   /* Reconnect timer */
int imcconnect_attempts;   /* How many times have we tried to reconnect? */
unsigned long imc_sequencenumber;   /* sequence# for outgoing packets */
bool imcpacketdebug = FALSE;
time_t imcucache_clock; /* prune ucache stuff regularly */
time_t imc_time;  /* Current clock time for the client */

void imclog( const char *format, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void imcbug( const char *format, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void imc_printf( CHAR_DATA * ch, const char *fmt, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void imcpager_printf( CHAR_DATA * ch, const char *fmt, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
char *imc_funcname( IMC_FUN * func );
IMC_FUN *imc_function( const char *func );
char *imc_send_social( CHAR_DATA * ch, char *argument, int telloption );
void imc_save_config( void );
void imc_save_channels( void );

char *const imcperm_names[] = {
   "Notset", "None", "Mort", "Imm", "Admin", "Imp"
};

SITEINFO *this_imcmud;
IMC_CHANNEL *first_imc_channel;
IMC_CHANNEL *last_imc_channel;
REMOTEINFO *first_rinfo;
REMOTEINFO *last_rinfo;
IMC_BAN *first_imc_ban;
IMC_BAN *last_imc_ban;
IMCUCACHE_DATA *first_imcucache;
IMCUCACHE_DATA *last_imcucache;
IMC_COLOR *first_imc_color;
IMC_COLOR *last_imc_color;
IMC_CMD_DATA *first_imc_command;
IMC_CMD_DATA *last_imc_command;
IMC_HELP_DATA *first_imc_help;
IMC_HELP_DATA *last_imc_help;
IMC_PHANDLER *first_phandler;
IMC_PHANDLER *last_phandler;
WHO_TEMPLATE *whot;

/*******************************************
 * String buffering and logging functions. *
 ******************************************/

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 *
 * Renamed so it can play itself system independent.
 * Samson 10-12-03
 */
size_t imcstrlcpy( char *dst, const char *src, size_t siz )
{
   register char *d = dst;
   register const char *s = src;
   register size_t n = siz;

   /*
    * Copy as many bytes as will fit
    */
   if( n != 0 && --n != 0 )
   {
      do
      {
         if( ( *d++ = *s++ ) == 0 )
            break;
      }
      while( --n != 0 );
   }

   /*
    * Not enough room in dst, add NUL and traverse rest of src
    */
   if( n == 0 )
   {
      if( siz != 0 )
         *d = '\0';  /* NUL-terminate dst */
      while( *s++ )
         ;
   }
   return ( s - src - 1 ); /* count does not include NUL */
}

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(initial dst) + strlen(src); if retval >= siz,
 * truncation occurred.
 *
 * Renamed so it can play itself system independent.
 * Samson 10-12-03
 */
size_t imcstrlcat( char *dst, const char *src, size_t siz )
{
   register char *d = dst;
   register const char *s = src;
   register size_t n = siz;
   size_t dlen;

   /*
    * Find the end of dst and adjust bytes left but don't go past end
    */
   while( n-- != 0 && *d != '\0' )
      d++;
   dlen = d - dst;
   n = siz - dlen;

   if( n == 0 )
      return ( dlen + strlen( s ) );
   while( *s != '\0' )
   {
      if( n != 1 )
      {
         *d++ = *s;
         n--;
      }
      s++;
   }
   *d = '\0';
   return ( dlen + ( s - src ) );   /* count does not include NUL */
}

/* Generic log function which will route the log messages to the appropriate system logging function */
void imclog( const char *format, ... )
{
   char buf[LGST], buf2[LGST];
   char *strtime;
   va_list ap;

   va_start( ap, format );
   vsnprintf( buf, LGST, format, ap );
   va_end( ap );

   snprintf( buf2, LGST, "IMC: %s", buf );

   strtime = ctime( &imc_time );
   strtime[strlen( strtime ) - 1] = '\0';
   fprintf( stderr, "%s :: %s\n", strtime, buf2 );

   return;
}

/* Generic bug logging function which will route the message to the appropriate function that handles bug logs */
void imcbug( const char *format, ... )
{
   char buf[LGST], buf2[LGST];
   char *strtime;
   va_list ap;

   va_start( ap, format );
   vsnprintf( buf, LGST, format, ap );
   va_end( ap );

   snprintf( buf2, LGST, "***BUG*** IMC: %s", buf );

   strtime = ctime( &imc_time );
   strtime[strlen( strtime ) - 1] = '\0';
   fprintf( stderr, "%s :: %s\n", strtime, buf2 );

   return;
}

/*
   Original Code from SW:FotE 1.1
   Reworked strrep function.
   Fixed a few glaring errors. It also will not overrun the bounds of a string.
   -- Xorith
*/
char *imcstrrep( const char *src, const char *sch, const char *rep )
{
   int lensrc = strlen( src ), lensch = strlen( sch ), lenrep = strlen( rep ), x, y, in_p;
   static char newsrc[LGST];
   bool searching = FALSE;

   newsrc[0] = '\0';
   for( x = 0, in_p = 0; x < lensrc; x++, in_p++ )
   {
      if( src[x] == sch[0] )
      {
         searching = TRUE;
         for( y = 0; y < lensch; y++ )
            if( src[x + y] != sch[y] )
               searching = FALSE;

         if( searching )
         {
            for( y = 0; y < lenrep; y++, in_p++ )
            {
               if( in_p == ( LGST - 1 ) )
               {
                  newsrc[in_p] = '\0';
                  return newsrc;
               }
               if( src[x - 1] == sch[0] )
               {
                  if( rep[0] == '\033' )
                  {
                     if( y < lensch )
                     {
                        if( y == 0 )
                           newsrc[in_p - 1] = sch[y];
                        else
                           newsrc[in_p] = sch[y];
                     }
                     else
                        y = lenrep;
                  }
                  else
                  {
                     if( y == 0 )
                        newsrc[in_p - 1] = rep[y];
                     newsrc[in_p] = rep[y];
                  }
               }
               else
                  newsrc[in_p] = rep[y];
            }
            x += lensch - 1;
            in_p--;
            searching = FALSE;
            continue;
         }
      }
      if( in_p == ( LGST - 1 ) )
      {
         newsrc[in_p] = '\0';
         return newsrc;
      }
      newsrc[in_p] = src[x];
   }
   newsrc[in_p] = '\0';
   return newsrc;
}

char *imcone_argument( char *argument, char *arg_first )
{
   char cEnd;
   int count;

   count = 0;

   if( arg_first )
      arg_first[0] = '\0';

   if( !argument || argument[0] == '\0' )
      return NULL;

   while( isspace( *argument ) )
      argument++;

   cEnd = ' ';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' && ++count <= 255 )
   {
      if( *argument == cEnd )
      {
         argument++;
         break;
      }

      if( arg_first )
         *arg_first++ = *argument++;
      else
         argument++;
   }

   if( arg_first )
      *arg_first = '\0';

   while( isspace( *argument ) )
      argument++;

   return argument;
}

/********************************
 * User level output functions. *
 *******************************/

char *imc_strip_colors( const char *txt )
{
   IMC_COLOR *color;
   static char tbuf[LGST];

   imcstrlcpy( tbuf, txt, LGST );
   for( color = first_imc_color; color; color = color->next )
      imcstrlcpy( tbuf, imcstrrep( tbuf, color->imctag, "" ), LGST );

   for( color = first_imc_color; color; color = color->next )
      imcstrlcpy( tbuf, imcstrrep( tbuf, color->mudtag, "" ), LGST );
   return tbuf;
}

/* Now tell me this isn't cleaner than the mess that was here before. -- Xorith */
/* Yes, Xorith it is. Now, how about this update? Much less hassle with no hardcoded table! -- Samson */
/* convert from imc color -> mud color */
char *color_itom( const char *txt, CHAR_DATA * ch )
{
   IMC_COLOR *color;
   static char tbuf[LGST];

   if( !txt || *txt == '\0' )
      return "";

   if( IMCIS_SET( IMCFLAG( ch ), IMC_COLORFLAG ) )
   {
      imcstrlcpy( tbuf, txt, LGST );
      for( color = first_imc_color; color; color = color->next )
         imcstrlcpy( tbuf, imcstrrep( tbuf, color->imctag, color->mudtag ), LGST );
   }
   else
      imcstrlcpy( tbuf, imc_strip_colors( txt ), LGST );

   return tbuf;
}

/* convert from mud color -> imc color */
char *color_mtoi( const char *txt )
{
   IMC_COLOR *color;
   static char tbuf[LGST];

   if( !txt || *txt == '\0' )
      return "";

   imcstrlcpy( tbuf, txt, LGST );
   for( color = first_imc_color; color; color = color->next )
      imcstrlcpy( tbuf, imcstrrep( tbuf, color->mudtag, color->imctag ), LGST );

   return tbuf;
}

/* Generic send_to_char type function to send to the proper code for each codebase */
void imc_to_char( const char *txt, CHAR_DATA * ch )
{
   char buf[LGST * 2];

   snprintf( buf, LGST * 2, "%s\033[0m", color_itom( txt, ch ) );
#if defined(IMCSMAUG)
   send_to_char_color( buf, ch );
#elif defined(IMCCIRCLE)
#if _CIRCLEMUD < CIRCLEMUD_VERSION(3,0,21)
   send_to_char( buf, ch );
#else
   send_to_char( ch, "%s", buf );
#endif
#elif defined(IMCSTANDALONE)
   fprintf( stderr, "%s\n", buf );
#else
   send_to_char( buf, ch );
#endif
   return;
}

/* Modified version of Smaug's ch_printf_color function */
void imc_printf( CHAR_DATA * ch, const char *fmt, ... )
{
   char buf[LGST];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, LGST, fmt, args );
   va_end( args );

   imc_to_char( buf, ch );
}

/* Generic send_to_pager type function to send to the proper code for each codebase */
void imc_to_pager( const char *txt, CHAR_DATA * ch )
{
   char buf[LGST * 2];

   snprintf( buf, LGST * 2, "%s\033[0m", color_itom( txt, ch ) );
#if defined(IMCSMAUG) || defined(IMCCHRONICLES)
   send_to_pager_color( buf, ch );
#elif defined(IMCROM)
   page_to_char( buf, ch );
#elif defined(IMC1STMUD)
   sendpage( ch, buf2 );
#else
   imc_to_char( buf, ch );
#endif
   return;
}

/* Generic pager_printf type function */
void imcpager_printf( CHAR_DATA * ch, const char *fmt, ... )
{
   char buf[LGST];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, LGST, fmt, args );
   va_end( args );

   imc_to_pager( buf, ch );
   return;
}

/********************************
 * Low level utility functions. *
 ********************************/

bool imcstr_prefix( const char *astr, const char *bstr )
{
   if( !astr )
   {
      imcbug( "Strn_cmp: null astr." );
      return TRUE;
   }

   if( !bstr )
   {
      imcbug( "Strn_cmp: null bstr." );
      return TRUE;
   }

   for( ; *astr; astr++, bstr++ )
   {
      if( LOWER( *astr ) != LOWER( *bstr ) )
         return TRUE;
   }
   return FALSE;
}

/*
 * Returns an initial-capped string.
 */
char *imccapitalize( const char *str )
{
   static char strcap[LGST];
   int i;

   for( i = 0; str[i] != '\0'; i++ )
      strcap[i] = tolower( str[i] );
   strcap[i] = '\0';
   strcap[0] = toupper( strcap[0] );
   return strcap;
}

/* Does the list have the member in it? */
bool imc_hasname( char *list, char *member )
{
   if( !list || list[0] == '\0' )
      return FALSE;

   if( !strstr( list, member ) )
      return FALSE;

   return TRUE;
}

/* Add a new member to the list, provided it's not already there */
void imc_addname( char **list, char *member )
{
   char newlist[LGST];

   if( imc_hasname( *list, member ) )
      return;

   if( !( *list ) || *list[0] == '\0' )
      imcstrlcpy( newlist, member, LGST );
   else
      snprintf( newlist, LGST, "%s %s", *list, member );

   IMCSTRFREE( *list );
   *list = IMCSTRALLOC( newlist );
   return;
}

/* Remove a member from a list, provided it's there. */
void imc_removename( char **list, char *member )
{
   char newlist[LGST];

   if( !imc_hasname( *list, member ) )
      return;

   imcstrlcpy( newlist, imcstrrep( *list, member, "" ), LGST );

   IMCSTRFREE( *list );
   *list = IMCSTRALLOC( newlist );
   return;
}

char *imc_nameof( char *src )
{
   static char name[SMST];
   size_t x;

   for( x = 0; x < strlen( src ); x++ )
   {
      if( src[x] == '@' )
         break;
      name[x] = src[x];
   }
   name[x] = '\0';

   return name;
}

char *imc_mudof( char *src )
{
   static char mud[SMST];
   char *person;

   if( !( person = strchr( src, '@' ) ) )
      imcstrlcpy( mud, src, SMST );
   else
      imcstrlcpy( mud, person + 1, SMST );
   return mud;
}

char *imc_channel_mudof( char *src )
{
   static char mud[SMST];
   size_t x;

   for( x = 0; x < strlen( src ); x++ )
   {
      if( src[x] == ':' )
      {
         mud[x] = '\0';
         break;
      }
      mud[x] = src[x];
   }
   return mud;
}

char *imc_channel_nameof( char *src )
{
   static char name[SMST];
   size_t x, y = 0;
   bool colon = FALSE;

   for( x = 0; x < strlen( src ); x++ )
   {
      if( src[x] == ':' )
      {
         colon = TRUE;
         continue;
      }
      if( !colon )
         continue;
      name[y++] = src[x];
   }
   name[x] = '\0';

   return name;
}

char *imc_makename( char *person, char *mud )
{
   static char name[SMST];

   snprintf( name, SMST, "%s@%s", person, mud );
   return name;
}

char *escape_string( const char *src )
{
   static char newstr[LGST];
   size_t x, y = 0;
   bool quote = FALSE, endquote = FALSE;

   if( strchr( src, ' ' ) )
   {
      quote = TRUE;
      endquote = TRUE;
   }

   for( x = 0; x < strlen( src ); x++ )
   {
      if( src[x] == '=' && quote )
      {
         newstr[y] = '=';
         newstr[++y] = '"';
         quote = FALSE;
      }
      else if( src[x] == '\n' )
      {
         newstr[y] = '\\';
         newstr[++y] = 'n';
      }
      else if( src[x] == '\r' )
      {
         newstr[y] = '\\';
         newstr[++y] = 'r';
      }
      else if( src[x] == '\\' )
      {
         newstr[y] = '\\';
         newstr[++y] = '\\';
      }
      else if( src[x] == '"' )
      {
         newstr[y] = '\\';
         newstr[++y] = '"';
      }
      else
         newstr[y] = src[x];
      y++;
   }

   if( endquote )
      newstr[y++] = '"';
   newstr[y] = '\0';
   return newstr;
}

/*
 * Returns a CHAR_DATA class which matches the string
 */
CHAR_DATA *imc_find_user( char *name )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *vch = NULL;

   for( d = first_descriptor; d; d = d->next )
   {
      if( ( vch = d->character ? d->character : d->original ) != NULL && !strcasecmp( CH_IMCNAME( vch ), name )
          && d->connected == CON_PLAYING )
         return vch;
   }
   return NULL;
}

char *imcgetname( char *from )
{
   static char buf[SMST];
   char *mud, *name;

   mud = imc_mudof( from );
   name = imc_nameof( from );

   if( !strcasecmp( mud, this_imcmud->localname ) )
      imcstrlcpy( buf, imc_nameof( name ), SMST );
   else
      imcstrlcpy( buf, from, SMST );

   return buf;
}

/* check if a packet from a given source should be ignored */
bool imc_isbanned( char *who )
{
   IMC_BAN *mud;

   for( mud = first_imc_ban; mud; mud = mud->next )
   {
      if( !strcasecmp( mud->name, imc_mudof( who ) ) )
         return TRUE;
   }
   return FALSE;
}

/* Beefed up to include wildcard ignores. */
bool imc_isignoring( CHAR_DATA * ch, const char *ignore )
{
   IMC_IGNORE *temp;

   /*
    * Wildcard support thanks to Xorith
    */
   for( temp = FIRST_IMCIGNORE( ch ); temp; temp = temp->next )
   {
#ifndef WIN32
      if( !fnmatch( temp->name, ignore, 0 ) )
#else
      if( !str_prefix( temp->name, ignore ) )
#endif
         return TRUE;
   }
   return FALSE;
}

/* There should only one of these..... */
void imc_delete_info( void )
{
   IMCSTRFREE( this_imcmud->servername );
   IMCSTRFREE( this_imcmud->rhost );
   IMCSTRFREE( this_imcmud->network );
   IMCSTRFREE( this_imcmud->clientpw );
   IMCSTRFREE( this_imcmud->serverpw );
   IMCDISPOSE( this_imcmud->outbuf );
   IMCSTRFREE( this_imcmud->localname );
   IMCSTRFREE( this_imcmud->fullname );
   IMCSTRFREE( this_imcmud->ihost );
   IMCSTRFREE( this_imcmud->email );
   IMCSTRFREE( this_imcmud->www );
   IMCSTRFREE( this_imcmud->details );
   IMCSTRFREE( this_imcmud->versionid );
   IMCSTRFREE( this_imcmud->base );
   IMCDISPOSE( this_imcmud );
}

/* delete the info entry "p" */
void imc_delete_reminfo( REMOTEINFO * p )
{
   IMCUNLINK( p, first_rinfo, last_rinfo, next, prev );
   IMCSTRFREE( p->name );
   IMCSTRFREE( p->version );
   IMCSTRFREE( p->network );
   IMCSTRFREE( p->path );
   IMCSTRFREE( p->url );
   IMCSTRFREE( p->port );
   IMCSTRFREE( p->host );
   IMCDISPOSE( p );
}

/* create a new info entry, insert into list */
void imc_new_reminfo( char *mud, char *version, char *netname, char *url, char *path )
{
   REMOTEINFO *p, *mud_prev;

   IMCCREATE( p, REMOTEINFO, 1 );

   p->name = IMCSTRALLOC( mud );

   if( !url || url[0] == '\0' )
      p->url = IMCSTRALLOC( "Unknown" );
   else
      p->url = IMCSTRALLOC( url );

   if( !version || version[0] == '\0' )
      p->version = IMCSTRALLOC( "Unknown" );
   else
      p->version = IMCSTRALLOC( version );

   if( !netname || netname[0] == '\0' )
      p->network = IMCSTRALLOC( this_imcmud->network );
   else
      p->network = IMCSTRALLOC( netname );

   if( !path || path[0] == '\0' )
      p->path = IMCSTRALLOC( "UNKNOWN" );
   else
      p->path = IMCSTRALLOC( path );

   p->expired = FALSE;

   for( mud_prev = first_rinfo; mud_prev; mud_prev = mud_prev->next )
      if( strcasecmp( mud_prev->name, mud ) >= 0 )
         break;

   if( !mud_prev )
      IMCLINK( p, first_rinfo, last_rinfo, next, prev );
   else
      IMCINSERT( p, mud_prev, first_rinfo, next, prev );

   return;
}

/* find an info entry for "name" */
REMOTEINFO *imc_find_reminfo( char *name )
{
   REMOTEINFO *p;

   for( p = first_rinfo; p; p = p->next )
   {
      if( !strcasecmp( name, p->name ) )
         return p;
   }
   return NULL;
}

bool check_mud( CHAR_DATA * ch, char *mud )
{
   REMOTEINFO *r = imc_find_reminfo( mud );

   if( !r )
   {
      imc_printf( ch, "~W%s ~cis not a valid mud name.\r\n", mud );
      return FALSE;
   }

   if( r->expired )
   {
      imc_printf( ch, "~W%s ~cis not connected right now.\r\n", r->name );
      return FALSE;
   }
   return TRUE;
}

bool check_mudof( CHAR_DATA * ch, char *mud )
{
   return check_mud( ch, imc_mudof( mud ) );
}

int get_imcpermvalue( const char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( imcperm_names ) / sizeof( imcperm_names[0] ) ); x++ )
      if( !strcasecmp( flag, imcperm_names[x] ) )
         return x;
   return -1;
}

bool imccheck_permissions( CHAR_DATA * ch, int checkvalue, int targetvalue, bool enforceequal )
{
   if( checkvalue < 0 || checkvalue > IMCPERM_IMP )
   {
      imc_to_char( "Invalid permission setting.\r\n", ch );
      return FALSE;
   }

   if( checkvalue > IMCPERM( ch ) )
   {
      imc_to_char( "You cannot set permissions higher than your own.\r\n", ch );
      return FALSE;
   }

   if( checkvalue == IMCPERM( ch ) && IMCPERM( ch ) != IMCPERM_IMP && enforceequal )
   {
      imc_to_char( "You cannot set permissions equal to your own. Someone higher up must do this.\r\n", ch );
      return FALSE;
   }

   if( IMCPERM( ch ) < targetvalue )
   {
      imc_to_char( "You cannot alter the permissions of someone or something above your own.\r\n", ch );
      return FALSE;
   }
   return TRUE;
}

IMC_BAN *imc_newban( void )
{
   IMC_BAN *ban;

   IMCCREATE( ban, IMC_BAN, 1 );
   ban->name = NULL;
   IMCLINK( ban, first_imc_ban, last_imc_ban, next, prev );
   return ban;
}

void imc_addban( char *what )
{
   IMC_BAN *ban;

   ban = imc_newban(  );
   ban->name = IMCSTRALLOC( what );
}

void imc_freeban( IMC_BAN * ban )
{
   IMCSTRFREE( ban->name );
   IMCUNLINK( ban, first_imc_ban, last_imc_ban, next, prev );
   IMCDISPOSE( ban );
}

bool imc_delban( const char *what )
{
   IMC_BAN *ban, *ban_next;

   for( ban = first_imc_ban; ban; ban = ban_next )
   {
      ban_next = ban->next;
      if( !strcasecmp( what, ban->name ) )
      {
         imc_freeban( ban );
         return TRUE;
      }
   }
   return FALSE;
}

IMC_CHANNEL *imc_findchannel( char *name )
{
   IMC_CHANNEL *c;

   for( c = first_imc_channel; c; c = c->next )
      if( ( c->name && !strcasecmp( c->name, name ) ) || ( c->local_name && !strcasecmp( c->local_name, name ) ) )
         return c;
   return NULL;
}

void imc_freechan( IMC_CHANNEL * c )
{
   int x;

   if( !c )
   {
      imcbug( "%s", "imc_freechan: Freeing NULL channel!" );
      return;
   }
   IMCUNLINK( c, first_imc_channel, last_imc_channel, next, prev );
   IMCSTRFREE( c->name );
   IMCSTRFREE( c->owner );
   IMCSTRFREE( c->operators );
   IMCSTRFREE( c->invited );
   IMCSTRFREE( c->excluded );
   IMCSTRFREE( c->local_name );
   IMCSTRFREE( c->regformat );
   IMCSTRFREE( c->emoteformat );
   IMCSTRFREE( c->socformat );

   for( x = 0; x < MAX_IMCHISTORY; x++ )
      IMCSTRFREE( c->history[x] );
   IMCDISPOSE( c );
   return;
}

void imcformat_channel( CHAR_DATA * ch, IMC_CHANNEL * d, int format, bool all )
{
   IMC_CHANNEL *c = NULL;
   char buf[LGST];

   if( all )
   {
      for( c = first_imc_channel; c; c = c->next )
      {
         if( !c->local_name || c->local_name[0] == '\0' )
            continue;

         if( format == 1 || format == 4 )
         {
            snprintf( buf, LGST, "~R[~Y%s~R] ~C%%s: ~c%%s", c->local_name );
            IMCSTRFREE( c->regformat );
            c->regformat = IMCSTRALLOC( buf );
         }
         if( format == 2 || format == 4 )
         {
            snprintf( buf, LGST, "~R[~Y%s~R] ~c%%s %%s", c->local_name );
            IMCSTRFREE( c->emoteformat );
            c->emoteformat = IMCSTRALLOC( buf );
         }
         if( format == 3 || format == 4 )
         {
            snprintf( buf, LGST, "~R[~Y%s~R] ~c%%s", c->local_name );
            IMCSTRFREE( c->socformat );
            c->socformat = IMCSTRALLOC( buf );
         }
      }
   }
   else
   {
      if( ch && ( !d->local_name || d->local_name[0] == '\0' ) )
      {
         imc_to_char( "This channel is not yet locally configured.\r\n", ch );
         return;
      }

      if( format == 1 || format == 4 )
      {
         snprintf( buf, LGST, "~R[~Y%s~R] ~C%%s: ~c%%s", d->local_name );
         IMCSTRFREE( d->regformat );
         d->regformat = IMCSTRALLOC( buf );
      }
      if( format == 2 || format == 4 )
      {
         snprintf( buf, LGST, "~R[~Y%s~R] ~c%%s %%s", d->local_name );
         IMCSTRFREE( d->emoteformat );
         d->emoteformat = IMCSTRALLOC( buf );
      }
      if( format == 3 || format == 4 )
      {
         snprintf( buf, LGST, "~R[~Y%s~R] ~c%%s", d->local_name );
         IMCSTRFREE( d->socformat );
         d->socformat = IMCSTRALLOC( buf );
      }
   }
   imc_save_channels(  );
   return;
}

void imc_new_channel( char *chan, char *owner, char *ops, char *invite, char *exclude, bool copen, int perm, char *lname )
{
   IMC_CHANNEL *c;

   if( !chan || chan[0] == '\0' )
   {
      imclog( "%s: NULL channel name received, skipping", __FUNCTION__ );
      return;
   }

   if( !strchr( chan, ':' ) )
   {
      imclog( "%s: Improperly formatted channel name: %s", __FUNCTION__, chan );
      return;
   }

   IMCCREATE( c, IMC_CHANNEL, 1 );
   c->name = IMCSTRALLOC( chan );
   c->owner = IMCSTRALLOC( owner );
   c->operators = IMCSTRALLOC( ops );
   c->invited = IMCSTRALLOC( invite );
   c->excluded = IMCSTRALLOC( exclude );

   if( lname && lname[0] != '\0' )
      c->local_name = IMCSTRALLOC( lname );

   else
      c->local_name = imc_channel_nameof( c->name );

   c->level = perm;
   c->refreshed = TRUE;
   c->open = copen;
   IMCLINK( c, first_imc_channel, last_imc_channel, next, prev );
   imcformat_channel( NULL, c, 4, FALSE );
   return;
}

/*
 * Read a number from a file. [Taken from Smaug's fread_number]
 */
int imcfread_number( FILE * fp )
{
   int number;
   bool sign;
   char c;

   do
   {
      if( feof( fp ) )
      {
         imclog( "%s", "imcfread_number: EOF encountered on read." );
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   sign = FALSE;
   if( c == '+' )
      c = getc( fp );
   else if( c == '-' )
   {
      sign = TRUE;
      c = getc( fp );
   }

   if( !isdigit( c ) )
   {
      imclog( "imcfread_number: bad format. (%c)", c );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         imclog( "%s", "imcfread_number: EOF encountered on read." );
         return number;
      }
      number = number * 10 + c - '0';
      c = getc( fp );
   }

   if( sign )
      number = 0 - number;

   if( c == '|' )
      number += imcfread_number( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/*
 * Read to end of line into static buffer [Taken from Smaug's fread_line]
 */
char *imcfread_line( FILE * fp )
{
   char line[LGST];
   char *pline;
   char c;
   int ln;

   pline = line;
   line[0] = '\0';
   ln = 0;

   /*
    * Skip blanks.
    * Read first char.
    */
   do
   {
      if( feof( fp ) )
      {
         imcbug( "%s", "imcfread_line: EOF encountered on read." );
         imcstrlcpy( line, "", LGST );
         return IMCSTRALLOC( line );
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   ungetc( c, fp );

   do
   {
      if( feof( fp ) )
      {
         imcbug( "%s", "imcfread_line: EOF encountered on read." );
         *pline = '\0';
         return IMCSTRALLOC( line );
      }
      c = getc( fp );
      *pline++ = c;
      ln++;
      if( ln >= ( LGST - 1 ) )
      {
         imcbug( "%s", "imcfread_line: line too long" );
         break;
      }
   }
   while( c != '\n' && c != '\r' );

   do
   {
      c = getc( fp );
   }
   while( c == '\n' || c == '\r' );

   ungetc( c, fp );
   pline--;
   *pline = '\0';

   /*
    * Since tildes generally aren't found at the end of lines, this seems workable. Will enable reading old configs.
    */
   if( line[strlen( line ) - 1] == '~' )
      line[strlen( line ) - 1] = '\0';
   return IMCSTRALLOC( line );
}

/*
 * Read one word (into static buffer). [Taken from Smaug's fread_word]
 */
char *imcfread_word( FILE * fp )
{
   static char word[SMST];
   char *pword;
   char cEnd;

   do
   {
      if( feof( fp ) )
      {
         imclog( "%s: EOF encountered on read.", __FUNCTION__ );
         word[0] = '\0';
         return word;
      }
      cEnd = getc( fp );
   }
   while( isspace( cEnd ) );

   if( cEnd == '\'' || cEnd == '"' )
   {
      pword = word;
   }
   else
   {
      word[0] = cEnd;
      pword = word + 1;
      cEnd = ' ';
   }

   for( ; pword < word + SMST; pword++ )
   {
      if( feof( fp ) )
      {
         imclog( "%s: EOF encountered on read.", __FUNCTION__ );
         *pword = '\0';
         return word;
      }
      *pword = getc( fp );
      if( cEnd == ' ' ? isspace( *pword ) : *pword == cEnd )
      {
         if( cEnd == ' ' )
            ungetc( *pword, fp );
         *pword = '\0';
         return word;
      }
   }
   imclog( "%s: word too long", __FUNCTION__ );
   return NULL;
}

/*
 * Read to end of line (for comments). [Taken from Smaug's fread_to_eol]
 */
void imcfread_to_eol( FILE * fp )
{
   char c;

   do
   {
      if( feof( fp ) )
      {
         imclog( "%s", "imcfread_to_eol: EOF encountered on read." );
         return;
      }
      c = getc( fp );
   }
   while( c != '\n' && c != '\r' );

   do
   {
      c = getc( fp );
   }
   while( c == '\n' || c == '\r' );

   ungetc( c, fp );
   return;
}

/*
 * Read a letter from a file. [Taken from Smaug's fread_letter]
 */
char imcfread_letter( FILE * fp )
{
   char c;

   do
   {
      if( feof( fp ) )
      {
         imclog( "%s", "imcfread_letter: EOF encountered on read." );
         return '\0';
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   return c;
}

/******************************************
 * Packet handling and routing functions. *
 ******************************************/

void imc_register_packet_handler( char *name, PACKET_FUN * func )
{
   IMC_PHANDLER *ph;

   for( ph = first_phandler; ph; ph = ph->next )
   {
      if( !strcasecmp( ph->name, name ) )
      {
         imclog( "Unable to register packet type %s. Another module has already registered it.", name );
         return;
      }
   }

   IMCCREATE( ph, IMC_PHANDLER, 1 );

   ph->name = IMCSTRALLOC( name );
   ph->func = func;

   IMCLINK( ph, first_phandler, last_phandler, next, prev );
   return;
}

void imc_freepacket( IMC_PACKET * p )
{
   IMC_PDATA *data, *data_next;

   for( data = p->first_data; data; data = data_next )
   {
      data_next = data->next;

      IMCUNLINK( data, p->first_data, p->last_data, next, prev );
      IMCDISPOSE( data );
   }
   IMCDISPOSE( p );
}

int find_next_esign( const char *string, int current )
{
   bool quote = FALSE;

   if( string[current] == '=' )
      current++;

   for( ; string[current] != '\0'; current++ )
   {
      if( string[current] == '\\' && string[current + 1] == '"' )
      {
         current++;
         continue;
      }

      if( string[current] == '"' )
         quote = !quote;

      if( string[current] == '=' && !quote )
         break;
   }

   if( string[current] == '\0' )
      return -1;

   return current;
}

char *imc_getData( char *output, const char *key, const char *packet )
{
   int current = 0;
   unsigned int i = 0;
   bool quote = FALSE;

   output[0] = '\0';

   if( !packet || packet[0] == '\0' || !key || key[0] == '\0' )
   {
      imcbug( "%s: Invalid input", __FUNCTION__ );
      return output;
   }

   while( ( current = find_next_esign( packet, current ) ) >= 0 )
   {
      if( strlen( key ) > ( unsigned int )current )
         continue;

      i = current - strlen( key );

      if( strncasecmp( &packet[i], key, strlen( key ) ) == 0 )
         break;
   }

   if( current < 0 )
      return output;

   current++;

   if( packet[current] == '"' )
   {
      quote = TRUE;
      current++;
   }

   for( i = 0; packet[current] != '\0'; current++ )
   {
      if( packet[current] == '"' && quote )
         break;

      if( packet[current] == ' ' && !quote )
         break;

      if( packet[current] != '\\' )
      {
         output[i++] = packet[current];
         continue;
      }
      current++;

      if( packet[current] == 'n' )
         output[i++] = '\n';
      else if( packet[current] == 'r' )
         output[i++] = '\r';
      else if( packet[current] == '"' )
         output[i++] = '"';
      else if( packet[current] == '\\' )
         output[i++] = '\\';
      else
         output[i++] = packet[current];
   }
   output[i] = '\0';
   return output;
}

void imc_write_buffer( const char *txt )
{
   char output[IMC_BUFF_SIZE];
   size_t length;

   /*
    * This should never happen
    */
   if( !this_imcmud || this_imcmud->desc < 1 )
   {
      imcbug( "%s: Configuration or socket is invalid!", __FUNCTION__ );
      return;
   }

   /*
    * This should never happen either
    */
   if( !this_imcmud->outbuf )
   {
      imcbug( "%s: Output buffer has not been allocated!", __FUNCTION__ );
      return;
   }

   snprintf( output, IMC_BUFF_SIZE, "%s\r\n", txt );
   length = strlen( output );

   /*
    * Expand the buffer as needed.
    */
   while( this_imcmud->outtop + length >= this_imcmud->outsize )
   {
      if( this_imcmud->outsize > 64000 )
      {
         /*
          * empty buffer
          */
         this_imcmud->outtop = 0;
         imcbug( "Buffer overflow: %ld. Purging.", this_imcmud->outsize );
         return;
      }
      this_imcmud->outsize *= 2;
      IMCRECREATE( this_imcmud->outbuf, char, this_imcmud->outsize );
   }

   /*
    * Copy.
    */
   strncpy( this_imcmud->outbuf + this_imcmud->outtop, output, length );   /* Leave this one alone! BAD THINGS(TM) will happen if you don't! */
   this_imcmud->outtop += length;
   this_imcmud->outbuf[this_imcmud->outtop] = '\0';
   return;
}

/*
 * Convert a packet to text to then send to the buffer
 */
void imc_write_packet( IMC_PACKET * p )
{
   IMC_PDATA *data;
   char txt[IMC_BUFF_SIZE];

   /*
    * Assemble your buffer, and at the same time disassemble the packet struct to free the memory
    */
   snprintf( txt, IMC_BUFF_SIZE, "%s %lu %s %s %s", p->from, ++imc_sequencenumber, this_imcmud->localname, p->type, p->to );
   for( data = p->first_data; data; data = data->next )
      snprintf( txt + strlen( txt ), IMC_BUFF_SIZE - strlen( txt ), "%s", data->field );
   imc_freepacket( p );

   imc_write_buffer( txt );
   return;
}

void imc_addtopacket( IMC_PACKET * p, const char *fmt, ... )
{
   IMC_PDATA *data;
   char pkt[IMC_BUFF_SIZE];
   va_list args;

   va_start( args, fmt );
   vsnprintf( pkt, IMC_BUFF_SIZE, fmt, args );
   va_end( args );

   IMCCREATE( data, IMC_PDATA, 1 );
   snprintf( data->field, IMC_BUFF_SIZE, " %s", escape_string( pkt ) );
   IMCLINK( data, p->first_data, p->last_data, next, prev );
   return;
}

IMC_PACKET *imc_newpacket( const char *from, const char *type, const char *to )
{
   IMC_PACKET *p;

   if( !type || type[0] == '\0' )
   {
      imcbug( "%s: Attempt to build packet with no type field.", __FUNCTION__ );
      return NULL;
   }

   if( !from || from[0] == '\0' )
   {
      imcbug( "%s: Attempt to build %s packet with no from field.", __FUNCTION__, type );
      return NULL;
   }

   if( !to || to[0] == '\0' )
   {
      imcbug( "%s: Attempt to build %s packet with no to field.", __FUNCTION__, type );
      return NULL;
   }

   IMCCREATE( p, IMC_PACKET, 1 );
   snprintf( p->from, SMST, "%s@%s", from, this_imcmud->localname );
   imcstrlcpy( p->type, type, SMST );
   imcstrlcpy( p->to, to, SMST );
   p->first_data = p->last_data = NULL;

   return p;
}

void imc_update_tellhistory( CHAR_DATA * ch, const char *msg )
{
   char new_msg[LGST];
   struct tm *local = localtime( &imc_time );
   int x;

   snprintf( new_msg, LGST, "~R[%-2.2d:%-2.2d] %s", local->tm_hour, local->tm_min, msg );

   for( x = 0; x < MAX_IMCTELLHISTORY; x++ )
   {
      if( IMCTELLHISTORY( ch, x ) == '\0' )
      {
         IMCTELLHISTORY( ch, x ) = IMCSTRALLOC( new_msg );
         break;
      }

      if( x == MAX_IMCTELLHISTORY - 1 )
      {
         int i;

         for( i = 1; i < MAX_IMCTELLHISTORY; i++ )
         {
            IMCSTRFREE( IMCTELLHISTORY( ch, i - 1 ) );
            IMCTELLHISTORY( ch, i - 1 ) = IMCSTRALLOC( IMCTELLHISTORY( ch, i ) );
         }
         IMCSTRFREE( IMCTELLHISTORY( ch, x ) );
         IMCTELLHISTORY( ch, x ) = IMCSTRALLOC( new_msg );
      }
   }
   return;
}

void imc_send_tell( char *from, char *to, char *txt, int reply )
{
   IMC_PACKET *p;

   p = imc_newpacket( from, "tell", to );
   imc_addtopacket( p, "text=%s", txt );
   if( reply > 0 )
      imc_addtopacket( p, "isreply=%d", reply );
   imc_write_packet( p );

   return;
}

PFUN( imc_recv_tell )
{
   CHAR_DATA *vic;
   char txt[LGST], isreply[SMST], buf[LGST];
   int reply;

   imc_getData( txt, "text", packet );
   imc_getData( isreply, "isreply", packet );
   reply = atoi( isreply );
   if( reply < 0 || reply > 2 )
      reply = 0;

   if( !( vic = imc_find_user( imc_nameof( q->to ) ) ) || IMCPERM( vic ) < IMCPERM_MORT )
   {
      snprintf( buf, LGST, "No player named %s exists here.", q->to );
      imc_send_tell( "*", q->from, buf, 1 );
      return;
   }

   if( strcasecmp( imc_nameof( q->from ), "ICE" ) )
   {
      if( IMCISINVIS( vic ) )
      {
         if( strcasecmp( imc_nameof( q->from ), "*" ) )
         {
            snprintf( buf, LGST, "%s is not receiving tells.", q->to );
            imc_send_tell( "*", q->from, buf, 1 );
         }
         return;
      }

      if( imc_isignoring( vic, q->from ) )
      {
         if( strcasecmp( imc_nameof( q->from ), "*" ) )
         {
            snprintf( buf, LGST, "%s is not receiving tells.", q->to );
            imc_send_tell( "*", q->from, buf, 1 );
         }
         return;
      }

      if( IMCIS_SET( IMCFLAG( vic ), IMC_TELL ) || IMCIS_SET( IMCFLAG( vic ), IMC_DENYTELL ) )
      {
         if( strcasecmp( imc_nameof( q->from ), "*" ) )
         {
            snprintf( buf, LGST, "%s is not receiving tells.", q->to );
            imc_send_tell( "*", q->from, buf, 1 );
         }
         return;
      }

      if( IMCAFK( vic ) )
      {
         if( strcasecmp( imc_nameof( q->from ), "*" ) )
         {
            snprintf( buf, LGST, "%s is currently AFK. Try back later.", q->to );
            imc_send_tell( "*", q->from, buf, 1 );
         }
         return;
      }

      if( strcasecmp( imc_nameof( q->from ), "*" ) )
      {
         IMCSTRFREE( IMC_RREPLY( vic ) );
         IMCSTRFREE( IMC_RREPLY_NAME( vic ) );
         IMC_RREPLY( vic ) = IMCSTRALLOC( q->from );
         IMC_RREPLY_NAME( vic ) = IMCSTRALLOC( imcgetname( q->from ) );
      }
   }

   /*
    * Tell social
    */
   if( reply == 2 )
      snprintf( buf, LGST, "~WImctell: ~c%s\r\n", txt );
   else
      snprintf( buf, LGST, "~C%s ~cimctells you ~c'~W%s~c'~!\r\n", imcgetname( q->from ), txt );
   imc_to_char( buf, vic );
   imc_update_tellhistory( vic, buf );
   return;
}

PFUN( imc_recv_emote )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *ch;
   char txt[LGST], lvl[SMST];
   int level;

   imc_getData( txt, "text", packet );
   imc_getData( lvl, "level", packet );

   level = get_imcpermvalue( lvl );
   if( level < 0 || level > IMCPERM_IMP )
      level = IMCPERM_IMM;

   for( d = first_descriptor; d; d = d->next )
   {
      if( d->connected == CON_PLAYING && ( ch = d->original ? d->original : d->character ) != NULL
          && IMCPERM( ch ) >= level )
         imc_printf( ch, "~p[~GIMC~p] %s %s\r\n", imcgetname( q->from ), txt );
   }
   return;
}

void update_imchistory( IMC_CHANNEL * channel, char *message )
{
   char msg[LGST], buf[LGST];
   struct tm *local;
   int x;

   if( !channel )
   {
      imcbug( "%s", "update_imchistory: NULL channel received!" );
      return;
   }

   if( !message || message[0] == '\0' )
   {
      imcbug( "%s", "update_imchistory: NULL message received!" );
      return;
   }

   imcstrlcpy( msg, message, LGST );
   for( x = 0; x < MAX_IMCHISTORY; x++ )
   {
      if( channel->history[x] == NULL )
      {
         local = localtime( &imc_time );
         snprintf( buf, LGST, "~R[%-2.2d/%-2.2d %-2.2d:%-2.2d] ~G%s",
                   local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, msg );
         channel->history[x] = IMCSTRALLOC( buf );

         if( IMCIS_SET( channel->flags, IMCCHAN_LOG ) )
         {
            FILE *fp;
            snprintf( buf, LGST, "%s%s.log", IMC_DIR, channel->local_name );
            if( !( fp = fopen( buf, "a" ) ) )
            {
               perror( buf );
               imcbug( "Could not open file %s!", buf );
            }
            else
            {
               fprintf( fp, "%s\n", imc_strip_colors( channel->history[x] ) );
               IMCFCLOSE( fp );
            }
         }
         break;
      }

      if( x == MAX_IMCHISTORY - 1 )
      {
         int y;

         for( y = 1; y < MAX_IMCHISTORY; y++ )
         {
            int z = y - 1;

            if( channel->history[z] != NULL )
            {
               IMCSTRFREE( channel->history[z] );
               channel->history[z] = IMCSTRALLOC( channel->history[y] );
            }
         }

         local = localtime( &imc_time );
         snprintf( buf, LGST, "~R[%-2.2d/%-2.2d %-2.2d:%-2.2d] ~G%s",
                   local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, msg );
         IMCSTRFREE( channel->history[x] );
         channel->history[x] = IMCSTRALLOC( buf );

         if( IMCIS_SET( channel->flags, IMCCHAN_LOG ) )
         {
            FILE *fp;
            snprintf( buf, LGST, "%s%s.log", IMC_DIR, channel->local_name );
            if( !( fp = fopen( buf, "a" ) ) )
            {
               perror( buf );
               imcbug( "Could not open file %s!", buf );
            }
            else
            {
               fprintf( fp, "%s\n", imc_strip_colors( channel->history[x] ) );
               IMCFCLOSE( fp );
            }
         }
      }
   }
   return;
}

void imc_display_channel( IMC_CHANNEL * c, const char *from, char *txt, int emote )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *ch;
   char buf[LGST], name[SMST];

   if( !c->local_name || c->local_name[0] == '\0' || !c->refreshed )
      return;

   if( emote < 2 )
      snprintf( buf, LGST, emote ? c->emoteformat : c->regformat, from, txt );
   else
      snprintf( buf, LGST, c->socformat, txt );

   for( d = first_descriptor; d; d = d->next )
   {
      ch = d->original ? d->original : d->character;

      if( !ch || d->connected != CON_PLAYING )
         continue;

#if !defined(IMCSTANDALONE)
      /*
       * Freaking stupid PC_DATA crap!
       */
      if( IS_NPC( ch ) )
         continue;
#endif

      if( IMCPERM( ch ) < c->level || !imc_hasname( IMC_LISTEN( ch ), c->local_name ) )
         continue;

      if( !c->open )
      {
         snprintf( name, SMST, "%s@%s", CH_IMCNAME( ch ), this_imcmud->localname );
         if( !imc_hasname( c->invited, name ) && strcasecmp( c->owner, name ) )
            continue;
      }
      imc_printf( ch, "%s\r\n", buf );
   }
   update_imchistory( c, buf );
}

PFUN( imc_recv_pbroadcast )
{
   IMC_CHANNEL *c;
   char chan[SMST], txt[LGST], emote[SMST], sender[SMST];
   int em;

   imc_getData( chan, "channel", packet );
   imc_getData( txt, "text", packet );
   imc_getData( emote, "emote", packet );
   imc_getData( sender, "realfrom", packet );

   em = atoi( emote );
   if( em < 0 || em > 2 )
      em = 0;

   if( !( c = imc_findchannel( chan ) ) )
      return;

   imc_display_channel( c, sender, txt, em );
   return;
}

PFUN( imc_recv_broadcast )
{
   IMC_CHANNEL *c;
   char chan[SMST], txt[LGST], emote[SMST], sender[SMST];
   int em;

   imc_getData( chan, "channel", packet );
   imc_getData( txt, "text", packet );
   imc_getData( emote, "emote", packet );
   imc_getData( sender, "sender", packet );

   em = atoi( emote );
   if( em < 0 || em > 2 )
      em = 0;

   if( !( c = imc_findchannel( chan ) ) )
      return;

   if( sender[0] == '\0' )
      imc_display_channel( c, q->from, txt, em );
   else
      imc_display_channel( c, sender, txt, em );
   return;
}

/* Send/recv private channel messages */
void imc_sendmessage( IMC_CHANNEL * c, char *name, char *text, int emote )
{
   IMC_PACKET *p;

   /*
    * Private channel
    */
   if( !c->open )
   {
      char to[SMST];

      snprintf( to, SMST, "IMC@%s", imc_channel_mudof( c->name ) );
      p = imc_newpacket( name, "ice-msg-p", to );
   }
   /*
    * Public channel
    */
   else
      p = imc_newpacket( name, "ice-msg-b", "*@*" );

   imc_addtopacket( p, "channel=%s", c->name );
   imc_addtopacket( p, "text=%s", text );
   imc_addtopacket( p, "emote=%d", emote );
   imc_addtopacket( p, "%s", "echo=1" );
   imc_write_packet( p );

   return;
}

PFUN( imc_recv_chanwhoreply )
{
   IMC_CHANNEL *c;
   CHAR_DATA *vic;
   char chan[SMST], list[IMC_BUFF_SIZE];

   imc_getData( chan, "channel", packet );
   imc_getData( list, "list", packet );

   if( !( c = imc_findchannel( chan ) ) )
      return;

   if( !( vic = imc_find_user( imc_nameof( q->to ) ) ) )
      return;

   imc_printf( vic, "~G%s", list );
   return;
}

char *get_local_chanwho( IMC_CHANNEL *c )
{
   DESCRIPTOR_DATA *d;
   CHAR_DATA *person;
   static char buf[IMC_BUFF_SIZE];
   int count = 0, col = 0;

   snprintf( buf, IMC_BUFF_SIZE, "The following people are listening to %s on %s:\r\n\r\n",
      c->local_name, this_imcmud->localname );

   for( d = first_descriptor; d; d = d->next )
   {
      person = d->original ? d->original : d->character;

      if( !person )
         continue;

      if( IMCISINVIS( person ) )
         continue;

      if( !imc_hasname( IMC_LISTEN( person ), c->local_name ) )
         continue;

      snprintf( buf + strlen( buf ), IMC_BUFF_SIZE - strlen( buf ), "%-15s", CH_IMCNAME( person ) );
      count++;
      if( ++col % 3 == 0 )
      {
         col = 0;
         snprintf( buf + strlen( buf ), IMC_BUFF_SIZE - strlen( buf ), "%s", "\r\n" );
      }
   }
   if( col != 0 )
      snprintf( buf + strlen( buf ), IMC_BUFF_SIZE - strlen( buf ), "%s", "\r\n" );
   /*
    * Send no response to a broadcast request if nobody is listening.
    */
   if( count == 0 )
      imcstrlcat( buf, "Nobody\r\n", IMC_BUFF_SIZE );
   else
      imcstrlcat( buf, "\r\n", IMC_BUFF_SIZE );
   return buf;
}

PFUN( imc_recv_chanwho )
{
   IMC_PACKET *p;
   IMC_CHANNEL *c;
   char buf[IMC_BUFF_SIZE], lvl[SMST], channel[SMST], lname[SMST];
   int level;

   imc_getData( lvl, "level", packet );
   level = get_imcpermvalue( lvl );
   if( level < 0 || level > IMCPERM_IMP )
      level = IMCPERM_ADMIN;

   imc_getData( channel, "channel", packet );
   imc_getData( lname, "lname", packet );

   if( !( c = imc_findchannel( channel ) ) )
      return;

   if( !c->local_name )
      snprintf( buf, IMC_BUFF_SIZE, "Channel %s is not locally configured on %s\r\n", lname, this_imcmud->localname );
   else if( c->level > level )
      snprintf( buf, IMC_BUFF_SIZE, "Channel %s is above your permission level on %s\r\n", lname, this_imcmud->localname );
   else
   {
      char cwho[IMC_BUFF_SIZE];

      imcstrlcpy( cwho, get_local_chanwho( c ), IMC_BUFF_SIZE );

      if( ( cwho[0] == '\0' || !strcasecmp( cwho, "" ) || !strcasecmp( cwho, "Nobody" ) ) && !strcasecmp( q->to, "*" ) )
         return;
      imcstrlcpy( buf, cwho, IMC_BUFF_SIZE );
   }

   p = imc_newpacket( "*", "ice-chan-whoreply", q->from );
   imc_addtopacket( p, "channel=%s", c->name );
   imc_addtopacket( p, "list=%s", buf );
   imc_write_packet( p );

   return;
}

void imc_sendnotify( CHAR_DATA * ch, char *chan, bool chon )
{
   IMC_PACKET *p;
   IMC_CHANNEL *channel;

   if( !IMCIS_SET( IMCFLAG( ch ), IMC_NOTIFY ) )
      return;

   if( !( channel = imc_findchannel( chan ) ) )
      return;

   p = imc_newpacket( CH_IMCNAME( ch ), "channel-notify", "*@*" );
   imc_addtopacket( p, "channel=%s", channel->name );
   imc_addtopacket( p, "status=%d", chon );
   imc_write_packet( p );

   return;
}

PFUN( imc_recv_channelnotify )
{
   IMC_CHANNEL *c;
   DESCRIPTOR_DATA *d;
   CHAR_DATA *ch;
   char buf[LGST];
   char chan[SMST], cstat[SMST];
   bool chon = FALSE;

   imc_getData( chan, "channel", packet );
   imc_getData( cstat, "status", packet );
   chon = atoi( cstat );

   if( !( c = imc_findchannel( chan ) ) )
      return;

   if( !c->local_name || c->local_name[0] == '\0' )
      return;

   if( chon == TRUE )
      snprintf( buf, LGST, c->emoteformat, q->from, "has joined the channel." );
   else
      snprintf( buf, LGST, c->emoteformat, q->from, "has left the channel." );

   for( d = first_descriptor; d; d = d->next )
   {
      ch = d->original ? d->original : d->character;

      if( !ch || d->connected != CON_PLAYING )
         continue;

#if !defined(IMCSTANDALONE)
      /*
       * Freaking stupid PC_DATA crap!
       */
      if( IS_NPC( ch ) )
         continue;
#endif

      if( IMCPERM( ch ) < c->level || !imc_hasname( IMC_LISTEN( ch ), c->local_name ) )
         continue;

      if( !IMCIS_SET( IMCFLAG( ch ), IMC_NOTIFY ) )
         continue;

      imc_printf( ch, "%s\r\n", buf );
   }
   return;
}

char *imccenterline( const char *string, int length )
{
   char stripped[300];
   static char outbuf[400];
   int amount;

   imcstrlcpy( stripped, imc_strip_colors( string ), 300 );
   amount = length - strlen( stripped );  /* Determine amount to put in front of line */

   if( amount < 1 )
      amount = 1;

   /*
    * Justice, you are the String God!
    */
   snprintf( outbuf, 400, "%*s%s%*s", ( amount / 2 ), "", string,
             ( ( amount / 2 ) * 2 ) == amount ? ( amount / 2 ) : ( ( amount / 2 ) + 1 ), "" );

   return outbuf;
}

char *imcrankbuffer( CHAR_DATA * ch )
{
   static char rbuf[SMST];

   if( IMCPERM( ch ) >= IMCPERM_IMM )
   {
      imcstrlcpy( rbuf, "~YStaff", SMST );

      if( CH_IMCRANK( ch ) && CH_IMCRANK( ch )[0] != '\0' )
         snprintf( rbuf, SMST, "~Y%s", color_mtoi( CH_IMCRANK( ch ) ) );
   }
   else
   {
      imcstrlcpy( rbuf, "~BPlayer", SMST );

      if( CH_IMCRANK( ch ) && CH_IMCRANK( ch )[0] != '\0' )
         snprintf( rbuf, SMST, "~B%s", color_mtoi( CH_IMCRANK( ch ) ) );
   }
   return rbuf;
}

void imc_send_whoreply( char *to, char *txt )
{
   IMC_PACKET *p;

   p = imc_newpacket( "*", "who-reply", to );
   imc_addtopacket( p, "text=%s", txt );
   imc_write_packet( p );

   return;
}

void imc_send_who( char *from, char *to, char *type )
{
   IMC_PACKET *p;

   p = imc_newpacket( from, "who", to );
   imc_addtopacket( p, "type=%s", type );
   imc_write_packet( p );

   return;
}

char *break_newlines( char *argument, char *arg_first )
{
   char cEnd;
   int count;

   count = 0;

   if( arg_first )
      arg_first[0] = '\0';

   if( !argument || argument[0] == '\0' )
      return NULL;

   while( isspace( *argument ) )
      argument++;

   cEnd = '\n';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' && ++count <= 255 )
   {
      if( *argument == cEnd )
      {
         argument++;
         break;
      }

      if( arg_first )
         *arg_first++ = *argument++;
      else
         argument++;
   }

   if( arg_first )
      *arg_first = '\0';

   while( isspace( *argument ) )
      argument++;

   return argument;
}

char *multiline_center( char *splitme )
{
   static char newline[LGST];
   char arg[SMST];

   newline[0] = '\0';
   while( 1 )
   {
      if( splitme[0] == '\0' )
         break;

      splitme = break_newlines( splitme, arg );

      if( strstr( arg, "<center>" ) )
      {
         imcstrlcpy( arg, imcstrrep( arg, "<center>", "" ), SMST );
         imcstrlcpy( arg, imccenterline( arg, 78 ), SMST );
      }
      imcstrlcat( newline, arg, LGST );
      imcstrlcat( newline, "\n", LGST );
   }
   return newline;
}

char *process_who_head( int plrcount )
{
   static char head[LGST];
   char pcount[SMST];

   imcstrlcpy( head, whot->head, LGST );
   snprintf( pcount, SMST, "%d", plrcount );

   imcstrlcpy( head, imcstrrep( head, "<%plrcount%>", pcount ), LGST );
   imcstrlcpy( head, multiline_center( head ), LGST );

   return head;
}

char *process_who_tail( int plrcount )
{
   static char tail[LGST];
   char pcount[SMST];

   imcstrlcpy( tail, whot->tail, LGST );
   snprintf( pcount, SMST, "%d", plrcount );

   imcstrlcpy( tail, imcstrrep( tail, "<%plrcount%>", pcount ), LGST );
   imcstrlcpy( tail, multiline_center( tail ), LGST );

   return tail;
}

char *process_plrline( char *plrrank, char *plrflags, char *plrname, char *plrtitle )
{
   static char pline[LGST];

   imcstrlcpy( pline, whot->immline, LGST );
   imcstrlcpy( pline, imcstrrep( pline, "<%charrank%>", plrrank ), LGST );
   imcstrlcpy( pline, imcstrrep( pline, "<%charflags%>", plrflags ), LGST );
   imcstrlcpy( pline, imcstrrep( pline, "<%charname%>", plrname ), LGST );
   imcstrlcpy( pline, imcstrrep( pline, "<%chartitle%>", plrtitle ), LGST );
   imcstrlcat( pline, "\n", LGST );

   return pline;
}

char *process_immline( char *plrrank, char *plrflags, char *plrname, char *plrtitle )
{
   static char pline[LGST];

   imcstrlcpy( pline, whot->immline, LGST );
   imcstrlcpy( pline, imcstrrep( pline, "<%charrank%>", plrrank ), LGST );
   imcstrlcpy( pline, imcstrrep( pline, "<%charflags%>", plrflags ), LGST );
   imcstrlcpy( pline, imcstrrep( pline, "<%charname%>", plrname ), LGST );
   imcstrlcpy( pline, imcstrrep( pline, "<%chartitle%>", plrtitle ), LGST );
   imcstrlcat( pline, "\n", LGST );

   return pline;
}

char *process_who_template( char *head, char *tail, char *plrlines, char *immlines, char *plrheader, char *immheader )
{
   static char master[LGST];

   imcstrlcpy( master, whot->master, LGST );
   imcstrlcpy( master, imcstrrep( master, "<%head%>", head ), LGST );
   imcstrlcpy( master, imcstrrep( master, "<%tail%>", tail ), LGST );
   imcstrlcpy( master, imcstrrep( master, "<%plrheader%>", plrheader ), LGST );
   imcstrlcpy( master, imcstrrep( master, "<%immheader%>", immheader ), LGST );
   imcstrlcpy( master, imcstrrep( master, "<%plrline%>", plrlines ), LGST );
   imcstrlcpy( master, imcstrrep( master, "<%immline%>", immlines ), LGST );

   return master;
}

char *imc_assemble_who( void )
{
   CHAR_DATA *person;
   DESCRIPTOR_DATA *d;
   int pcount = 0;
   bool plr = FALSE, imm = FALSE;
   char plrheader[SMST], immheader[SMST], rank[SMST], flags[SMST], name[SMST], title[SMST], plrline[SMST], immline[SMST];
   char plrlines[LGST], immlines[LGST], head[LGST], tail[LGST];
   static char master[LGST]; /* The final result that gets returned */

   plrlines[0] = '\0';
   immlines[0] = '\0';
   plrheader[0] = '\0';
   immheader[0] = '\0';

   for( d = first_descriptor; d; d = d->next )
   {
      person = d->original ? d->original : d->character;

      if( person && d->connected == CON_PLAYING )
      {
         if( IMCPERM( person ) <= IMCPERM_NONE || IMCPERM( person ) >= IMCPERM_IMM )
            continue;

         if( IMCISINVIS( person ) )
            continue;

         ++pcount;

         if( !plr )
         {
            imcstrlcpy( plrheader, whot->plrheader, LGST );
            plr = TRUE;
         }

         imcstrlcpy( rank, imcrankbuffer( person ), LGST );

         if( IMCAFK( person ) )
            imcstrlcpy( flags, "AFK", SMST );
         else
            imcstrlcpy( flags, "---", SMST );

         imcstrlcpy( name, CH_IMCNAME( person ), SMST );
         imcstrlcpy( title, color_mtoi( CH_IMCTITLE( person ) ), SMST );
         imcstrlcpy( plrline, process_plrline( rank, flags, name, title ), SMST );
         imcstrlcat( plrlines, plrline, LGST );
      }
   }

   imm = FALSE;
   for( d = first_descriptor; d; d = d->next )
   {
      person = d->original ? d->original : d->character;

      if( person && d->connected == CON_PLAYING )
      {
         if( IMCPERM( person ) <= IMCPERM_NONE || IMCPERM( person ) < IMCPERM_IMM )
            continue;

         if( IMCISINVIS( person ) )
            continue;

         ++pcount;

         if( !imm )
         {
            imcstrlcpy( immheader, whot->immheader, SMST );
            imm = TRUE;
         }

         imcstrlcpy( rank, imcrankbuffer( person ), SMST );

         if( IMCAFK( person ) )
            imcstrlcpy( flags, "AFK", SMST );
         else
            imcstrlcpy( flags, "---", SMST );

         imcstrlcpy( name, CH_IMCNAME( person ), SMST );
         imcstrlcpy( title, color_mtoi( CH_IMCTITLE( person ) ), SMST );
         imcstrlcpy( immline, process_immline( rank, flags, name, title ), SMST );
         imcstrlcat( immlines, immline, LGST );
      }
   }

   imcstrlcpy( head, process_who_head( pcount ), LGST );
   imcstrlcpy( tail, process_who_tail( pcount ), LGST );
   imcstrlcpy( master, process_who_template( head, tail, plrlines, immlines, plrheader, immheader ), LGST );

   return master;
}

void imc_process_who( char *from )
{
   char whoreply[IMC_BUFF_SIZE];

   imcstrlcpy( whoreply, imc_assemble_who(), IMC_BUFF_SIZE );
   imc_send_whoreply( from, whoreply );
}

/* Finger code */
void imc_process_finger( char *from, char *type )
{
   CHAR_DATA *victim;
   char buf[IMC_BUFF_SIZE], to[SMST];

   if( !type || type[0] == '\0' )
      return;

   type = imcone_argument( type, to );
   if( !( victim = imc_find_user( type ) ) )
   {
      imc_send_whoreply( from, "No such player is online.\r\n" );
      return;
   }

   if( IMCISINVIS( victim ) || IMCPERM( victim ) < IMCPERM_MORT )
   {
      imc_send_whoreply( from, "No such player is online.\r\n" );
      return;
   }

   snprintf( buf, IMC_BUFF_SIZE,
             "\r\n~cPlayer Profile for ~W%s~c:\r\n"
             "~W-------------------------------\r\n"
             "~cStatus: ~W%s\r\n"
             "~cPermission level: ~W%s\r\n"
             "~cListening to channels [Names may not match your mud]: ~W%s\r\n",
             CH_IMCNAME( victim ), ( IMCAFK( victim ) ? "AFK" : "Lurking about" ),
             imcperm_names[IMCPERM( victim )],
             ( IMC_LISTEN( victim ) && IMC_LISTEN( victim )[0] != '\0' ) ? IMC_LISTEN( victim ) : "None" );

   if( !IMCIS_SET( IMCFLAG( victim ), IMC_PRIVACY ) )
      snprintf( buf + strlen( buf ), IMC_BUFF_SIZE - strlen( buf ),
                "~cEmail   : ~W%s\r\n"
                "~cHomepage: ~W%s\r\n"
                "~cICQ     : ~W%d\r\n"
                "~cAIM     : ~W%s\r\n"
                "~cYahoo   : ~W%s\r\n"
                "~cMSN     : ~W%s\r\n",
                ( IMC_EMAIL( victim ) && IMC_EMAIL( victim )[0] != '\0' ) ? IMC_EMAIL( victim ) : "None",
                ( IMC_HOMEPAGE( victim ) && IMC_HOMEPAGE( victim )[0] != '\0' ) ? IMC_HOMEPAGE( victim ) : "None",
                IMC_ICQ( victim ),
                ( IMC_AIM( victim ) && IMC_AIM( victim )[0] != '\0' ) ? IMC_AIM( victim ) : "None",
                ( IMC_YAHOO( victim ) && IMC_YAHOO( victim )[0] != '\0' ) ? IMC_YAHOO( victim ) : "None",
                ( IMC_MSN( victim ) && IMC_MSN( victim )[0] != '\0' ) ? IMC_MSN( victim ) : "None" );

   snprintf( buf + strlen( buf ), IMC_BUFF_SIZE - strlen( buf ), "~W%s\r\n",
             ( IMC_COMMENT( victim ) && IMC_COMMENT( victim )[0] != '\0' ) ? IMC_COMMENT( victim ) : "" );

   imc_send_whoreply( from, buf );
}

PFUN( imc_recv_who )
{
   char type[SMST], buf[IMC_BUFF_SIZE];

   imc_getData( type, "type", packet );

   if( !strcasecmp( type, "who" ) )
   {
      imc_process_who( q->from );
      return;
   }
   else if( strstr( type, "finger" ) )
   {
      imc_process_finger( q->from, type );
      return;
   }
   else if( !strcasecmp( type, "info" ) )
   {
      snprintf( buf, IMC_BUFF_SIZE, "\r\n~WMUD Name    : ~c%s\r\n", this_imcmud->localname );
      snprintf( buf + strlen( buf ), IMC_BUFF_SIZE - strlen( buf ), "~WHost        : ~c%s\r\n", this_imcmud->ihost );
      snprintf( buf + strlen( buf ), IMC_BUFF_SIZE - strlen( buf ), "~WAdmin Email : ~c%s\r\n", this_imcmud->email );
      snprintf( buf + strlen( buf ), IMC_BUFF_SIZE - strlen( buf ), "~WWebsite     : ~c%s\r\n", this_imcmud->www );
      snprintf( buf + strlen( buf ), IMC_BUFF_SIZE - strlen( buf ), "~WIMC2 Version: ~c%s\r\n", this_imcmud->versionid );
      snprintf( buf + strlen( buf ), IMC_BUFF_SIZE - strlen( buf ), "~WDetails     : ~c%s\r\n", this_imcmud->details );
   }
   else
      snprintf( buf, IMC_BUFF_SIZE, "%s is not a valid option. Options are: who, finger, or info.\r\n", type );

   imc_send_whoreply( q->from, buf );
   return;
}

PFUN( imc_recv_whoreply )
{
   CHAR_DATA *vic;
   char txt[IMC_BUFF_SIZE];

   if( !( vic = imc_find_user( imc_nameof( q->to ) ) ) )
      return;

   imc_getData( txt, "text", packet );
   imc_to_pager( txt, vic );
   return;
}

void imc_send_whoisreply( char *to, char *data )
{
   IMC_PACKET *p;

   p = imc_newpacket( "*", "whois-reply", to );
   imc_addtopacket( p, "text=%s", data );
   imc_write_packet( p );
   return;
}

PFUN( imc_recv_whoisreply )
{
   CHAR_DATA *vic;
   char txt[LGST];

   imc_getData( txt, "text", packet );

   if( ( vic = imc_find_user( imc_nameof( q->to ) ) ) != NULL )
      imc_to_char( txt, vic );
   return;
}

void imc_send_whois( char *from, char *user )
{
   IMC_PACKET *p;

   p = imc_newpacket( from, "whois", user );
   imc_write_packet( p );
   return;
}

PFUN( imc_recv_whois )
{
   CHAR_DATA *vic;
   char buf[LGST];

   if( ( vic = imc_find_user( imc_nameof( q->to ) ) ) != NULL && !IMCISINVIS( vic ) )
   {
      snprintf( buf, LGST, "~RIMC Locate: ~Y%s@%s: ~cOnline.\r\n", CH_IMCNAME( vic ), this_imcmud->localname );
      imc_send_whoisreply( q->from, buf );
   }
   return;
}

PFUN( imc_recv_beep )
{
   CHAR_DATA *vic = NULL;
   char buf[LGST];

   if( !( vic = imc_find_user( imc_nameof( q->to ) ) ) || IMCPERM( vic ) < IMCPERM_MORT )
   {
      snprintf( buf, LGST, "No player named %s exists here.", q->to );
      imc_send_tell( "*", q->from, buf, 1 );
      return;
   }

   if( IMCISINVIS( vic ) )
   {
      if( strcasecmp( imc_nameof( q->from ), "*" ) )
      {
         snprintf( buf, LGST, "%s is not receiving beeps.", q->to );
         imc_send_tell( "*", q->from, buf, 1 );
      }
      return;
   }

   if( imc_isignoring( vic, q->from ) )
   {
      if( strcasecmp( imc_nameof( q->from ), "*" ) )
      {
         snprintf( buf, LGST, "%s is not receiving beeps.", q->to );
         imc_send_tell( "*", q->from, buf, 1 );
      }
      return;
   }

   if( IMCIS_SET( IMCFLAG( vic ), IMC_BEEP ) || IMCIS_SET( IMCFLAG( vic ), IMC_DENYBEEP ) )
   {
      if( strcasecmp( imc_nameof( q->from ), "*" ) )
      {
         snprintf( buf, LGST, "%s is not receiving beeps.", q->to );
         imc_send_tell( "*", q->from, buf, 1 );
      }
      return;
   }

   if( IMCAFK( vic ) )
   {
      if( strcasecmp( imc_nameof( q->from ), "*" ) )
      {
         snprintf( buf, LGST, "%s is currently AFK. Try back later.", q->to );
         imc_send_tell( "*", q->from, buf, 1 );
      }
      return;
   }

   /*
    * always display the TRUE name here
    */
   imc_printf( vic, "~c\a%s imcbeeps you.~!\r\n", q->from );
}

void imc_send_beep( char *from, char *to )
{
   IMC_PACKET *p;

   p = imc_newpacket( from, "beep", to );
   imc_write_packet( p );

   return;
}

PFUN( imc_recv_isalive )
{
   REMOTEINFO *r;
   char version[SMST], netname[SMST], url[SMST], host[SMST], iport[SMST];

   imc_getData( version, "versionid", packet );
   imc_getData( netname, "networkname", packet );
   imc_getData( url, "url", packet );
   imc_getData( host, "host", packet );
   imc_getData( iport, "port", packet );

   if( !( r = imc_find_reminfo( imc_mudof( q->from ) ) ) )
   {
      imc_new_reminfo( imc_mudof( q->from ), version, netname, url, q->route );
      return;
   }

   r->expired = FALSE;

   if( url[0] != '\0' )
   {
      IMCSTRFREE( r->url );
      r->url = IMCSTRALLOC( url );
   }

   if( version[0] != '\0' )
   {
      IMCSTRFREE( r->version );
      r->version = IMCSTRALLOC( version );
   }

   if( netname[0] != '\0' )
   {
      IMCSTRFREE( r->network );
      r->network = IMCSTRALLOC( netname );
   }

   if( q->route && q->route[0] != '\0' )
   {
      IMCSTRFREE( r->path );
      r->path = IMCSTRALLOC( q->route );
   }

   if( host[0] != '\0' )
   {
      IMCSTRFREE( r->host );
      r->host = IMCSTRALLOC( host );
   }

   if( iport[0] != '\0' )
   {
      IMCSTRFREE( r->port );
      r->port = IMCSTRALLOC( iport );
   }
   return;
}

PFUN( imc_send_keepalive )
{
   IMC_PACKET *p;

   if( q )
      p = imc_newpacket( "*", "is-alive", q->from );
   else
      p = imc_newpacket( "*", "is-alive", packet );
   imc_addtopacket( p, "versionid=%s", this_imcmud->versionid );
   imc_addtopacket( p, "url=%s", this_imcmud->www );
   imc_addtopacket( p, "host=%s", this_imcmud->ihost );
   imc_addtopacket( p, "port=%d", this_imcmud->iport );
   imc_write_packet( p );

   return;
}

void imc_request_keepalive( void )
{
   IMC_PACKET *p;

   p = imc_newpacket( "*", "keepalive-request", "*@*" );
   imc_write_packet( p );

   imc_send_keepalive( NULL, "*@*" );
   return;
}

void imc_firstrefresh( void )
{
   IMC_PACKET *p;

   p = imc_newpacket( "*", "ice-refresh", "IMC@$" );
   imc_write_packet( p );

   return;
}

PFUN( imc_recv_iceupdate )
{
   IMC_CHANNEL *c;
   char chan[SMST], owner[SMST], ops[SMST], invite[SMST], exclude[SMST], policy[SMST], level[SMST], lname[SMST];
   int perm;
   bool copen;

   imc_getData( chan, "channel", packet );
   imc_getData( owner, "owner", packet );
   imc_getData( ops, "operators", packet );
   imc_getData( invite, "invited", packet );
   imc_getData( exclude, "excluded", packet );
   imc_getData( policy, "policy", packet );
   imc_getData( level, "level", packet );
   imc_getData( lname, "localname", packet );

   if( !strcasecmp( policy, "open" ) )
      copen = TRUE;
   else
      copen = FALSE;

   perm = get_imcpermvalue( level );
   if( perm < 0 || perm > IMCPERM_IMP )
      perm = IMCPERM_ADMIN;

   if( !( c = imc_findchannel( chan ) ) )
   {
      imc_new_channel( chan, owner, ops, invite, exclude, copen, perm, lname );
      return;
   }

   if( chan[0] == '\0' )
   {
      imclog( "%s: NULL channel name received, skipping", __FUNCTION__ );
      return;
   }

   IMCSTRFREE( c->name );
   IMCSTRFREE( c->owner );
   IMCSTRFREE( c->operators );
   IMCSTRFREE( c->invited );
   IMCSTRFREE( c->excluded );

   c->name = IMCSTRALLOC( chan );
   c->owner = IMCSTRALLOC( owner );
   c->operators = IMCSTRALLOC( ops );
   c->invited = IMCSTRALLOC( invite );
   c->excluded = IMCSTRALLOC( exclude );
   c->open = copen;
   if( c->level == IMCPERM_NOTSET )
      c->level = perm;

   c->refreshed = TRUE;
   return;
}

PFUN( imc_recv_icedestroy )
{
   IMC_CHANNEL *c;
   char chan[SMST];

   imc_getData( chan, "channel", packet );

   if( !( c = imc_findchannel( chan ) ) )
      return;

   imc_freechan( c );
   imc_save_channels(  );
}

int imctodikugender( int gender )
{
   int sex = 0;

   if( gender == 0 )
      sex = SEX_MALE;

   if( gender == 1 )
      sex = SEX_FEMALE;

   if( gender > 1 )
      sex = SEX_NEUTRAL;

   return sex;
}

int dikutoimcgender( int gender )
{
   int sex = 0;

   if( gender > 2 || gender < 0 )
      sex = 2;

   if( gender == SEX_MALE )
      sex = 0;

   if( gender == SEX_FEMALE )
      sex = 1;

   return sex;
}

int imc_get_ucache_gender( const char *name )
{
   IMCUCACHE_DATA *user;

   for( user = first_imcucache; user; user = user->next )
   {
      if( !strcasecmp( user->name, name ) )
         return user->gender;
   }

   /*
    * -1 means you aren't in the list and need to be put there.
    */
   return -1;
}

/* Saves the ucache info to disk because it would just be spamcity otherwise */
void imc_save_ucache( void )
{
   FILE *fp;
   IMCUCACHE_DATA *user;

   if( !( fp = fopen( IMC_UCACHE_FILE, "w" ) ) )
   {
      imclog( "%s", "Couldn't write to IMC2 ucache file." );
      return;
   }

   for( user = first_imcucache; user; user = user->next )
   {
      fprintf( fp, "%s", "#UCACHE\n" );
      fprintf( fp, "Name %s\n", user->name );
      fprintf( fp, "Sex  %d\n", user->gender );
      fprintf( fp, "Time %ld\n", ( long int )user->time );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   IMCFCLOSE( fp );
   return;
}

void imc_prune_ucache( void )
{
   IMCUCACHE_DATA *ucache, *next_ucache;

   for( ucache = first_imcucache; ucache; ucache = next_ucache )
   {
      next_ucache = ucache->next;

      /*
       * Info older than 30 days is removed since this person likely hasn't logged in at all
       */
      if( imc_time - ucache->time >= 2592000 )
      {
         IMCSTRFREE( ucache->name );
         IMCUNLINK( ucache, first_imcucache, last_imcucache, next, prev );
         IMCDISPOSE( ucache );
      }
   }
   imc_save_ucache(  );
   return;
}

/* Updates user info if they exist, adds them if they don't. */
void imc_ucache_update( const char *name, int gender )
{
   IMCUCACHE_DATA *user;

   for( user = first_imcucache; user; user = user->next )
   {
      if( !strcasecmp( user->name, name ) )
      {
         user->gender = gender;
         user->time = imc_time;
         return;
      }
   }
   IMCCREATE( user, IMCUCACHE_DATA, 1 );
   user->name = IMCSTRALLOC( name );
   user->gender = gender;
   user->time = imc_time;
   IMCLINK( user, first_imcucache, last_imcucache, next, prev );

   imc_save_ucache(  );
   return;
}

void imc_send_ucache_update( const char *visname, int gender )
{
   IMC_PACKET *p;

   p = imc_newpacket( visname, "user-cache", "*@*" );
   imc_addtopacket( p, "gender=%d", gender );

   imc_write_packet( p );
   return;
}

PFUN( imc_recv_ucache )
{
   char gen[SMST];
   int sex, gender;

   imc_getData( gen, "gender", packet );
   gender = atoi( gen );

   sex = imc_get_ucache_gender( q->from );

   if( sex == gender )
      return;

   imc_ucache_update( q->from, gender );
   return;
}

void imc_send_ucache_request( char *targetuser )
{
   IMC_PACKET *p;
   char to[SMST];

   snprintf( to, SMST, "*@%s", imc_mudof( targetuser ) );
   p = imc_newpacket( "*", "user-cache-request", to );
   imc_addtopacket( p, "user=%s", targetuser );
   imc_write_packet( p );

   return;
}

PFUN( imc_recv_ucache_request )
{
   IMC_PACKET *p;
   char to[SMST], user[SMST];
   int gender;

   imc_getData( user, "user", packet );
   gender = imc_get_ucache_gender( user );

   /*
    * Gender of -1 means they aren't in the mud's ucache table. Don't waste the reply packet.
    */
   if( gender == -1 )
      return;

   snprintf( to, SMST, "*@%s", imc_mudof( q->from ) );
   p = imc_newpacket( "*", "user-cache-reply", to );
   imc_addtopacket( p, "user=%s", user );
   imc_addtopacket( p, "gender=%d", gender );
   imc_write_packet( p );

   return;
}

PFUN( imc_recv_ucache_reply )
{
   char user[SMST], gen[SMST];
   int sex, gender;

   imc_getData( user, "user", packet );
   imc_getData( gen, "gender", packet );
   gender = atoi( gen );

   sex = imc_get_ucache_gender( user );

   if( sex == gender )
      return;

   imc_ucache_update( user, gender );
   return;
}

PFUN( imc_recv_closenotify )
{
   REMOTEINFO *r;
   char host[SMST];

   imc_getData( host, "host", packet );

   if( !( r = imc_find_reminfo( host ) ) )
      return;

   r->expired = TRUE;
   return;
}

void imc_register_default_packets( void )
{
   /*
    * Once registered, these are not cleared unless the mud is shut down
    */
   if( first_phandler )
      return;

   imc_register_packet_handler( "keepalive-request", imc_send_keepalive );
   imc_register_packet_handler( "is-alive", imc_recv_isalive );
   imc_register_packet_handler( "ice-update", imc_recv_iceupdate );
   imc_register_packet_handler( "ice-msg-r", imc_recv_pbroadcast );
   imc_register_packet_handler( "ice-msg-b", imc_recv_broadcast );
   imc_register_packet_handler( "user-cache", imc_recv_ucache );
   imc_register_packet_handler( "user-cache-request", imc_recv_ucache_request );
   imc_register_packet_handler( "user-cache-reply", imc_recv_ucache_reply );
   imc_register_packet_handler( "tell", imc_recv_tell );
   imc_register_packet_handler( "emote", imc_recv_emote );
   imc_register_packet_handler( "ice-destroy", imc_recv_icedestroy );
   imc_register_packet_handler( "who", imc_recv_who );
   imc_register_packet_handler( "who-reply", imc_recv_whoreply );
   imc_register_packet_handler( "whois", imc_recv_whois );
   imc_register_packet_handler( "whois-reply", imc_recv_whoisreply );
   imc_register_packet_handler( "beep", imc_recv_beep );
   imc_register_packet_handler( "ice-chan-who", imc_recv_chanwho );
   imc_register_packet_handler( "ice-chan-whoreply", imc_recv_chanwhoreply );
   imc_register_packet_handler( "channel-notify", imc_recv_channelnotify );
   imc_register_packet_handler( "close-notify", imc_recv_closenotify );
}

PACKET_FUN *pfun_lookup( const char *type )
{
   IMC_PHANDLER *ph;

   for( ph = first_phandler; ph; ph = ph->next )
      if( !strcasecmp( type, ph->name ) )
         return ph->func;

   return NULL;
}

void imc_parse_packet( char *packet )
{
   IMC_PACKET *p;
   PACKET_FUN *pfun;
   char arg[SMST];
   unsigned long seq;

   IMCCREATE( p, IMC_PACKET, 1 );

   packet = imcone_argument( packet, p->from );
   packet = imcone_argument( packet, arg );
   seq = atol( arg );

   packet = imcone_argument( packet, p->route );
   packet = imcone_argument( packet, p->type );
   packet = imcone_argument( packet, p->to );

   /* Banned muds are silently dropped - thanks to WynterNyght@IoG for noticing this was missing. */
   if( imc_isbanned( p->from ) )
   {
      IMCDISPOSE( p );
      return;
   }

   pfun = pfun_lookup( p->type );
   if( !pfun )
   {
      if( imcpacketdebug )
      {
         imclog( "PACKET: From %s, Seq %lu, Route %s, Type %s, To %s, EXTRA %s",
                 p->from, seq, p->route, p->type, p->to, packet );
         imclog( "No packet handler function has been defined for %s", p->type );
      }
      IMCDISPOSE( p );
      return;
   }
   ( *pfun ) ( p, packet );

   /*
    * This might seem slow, but we need to track muds who don't send is-alive packets
    */
   if( !( imc_find_reminfo( imc_mudof( p->from ) ) ) )
      imc_new_reminfo( imc_mudof( p->from ), "Unknown", this_imcmud->network, "Unknown", p->route );

   IMCDISPOSE( p );
   return;
}

void imc_finalize_connection( char *name, char *netname )
{
   this_imcmud->state = IMC_ONLINE;

   if( netname && netname[0] != '\0' )
   {
      IMCSTRFREE( this_imcmud->network );
      this_imcmud->network = IMCSTRALLOC( netname );
   }

   IMCSTRFREE( this_imcmud->servername );
   this_imcmud->servername = IMCSTRALLOC( name );

   imclog( "Connected to %s. Network ID: %s", name, ( netname && netname[0] != '\0' ) ? netname : "Unknown" );

   imcconnect_attempts = 0;
   imc_request_keepalive(  );
   imc_firstrefresh(  );
   return;
}

/* Handle an autosetup response from a supporting server - Samson 8-12-03 */
void imc_handle_autosetup( char *source, char *servername, char *cmd, char *txt, char *encrypt )
{
   if( !strcasecmp( cmd, "reject" ) )
   {
      if( !strcasecmp( txt, "connected" ) )
      {
         imclog( "There is already a mud named %s connected to the network.", this_imcmud->localname );
         imc_shutdown( FALSE );
         return;
      }
      if( !strcasecmp( txt, "private" ) )
      {
         imclog( "%s is a private server. Autosetup denied.", servername );
         imc_shutdown( FALSE );
         return;
      }
      if( !strcasecmp( txt, "full" ) )
      {
         imclog( "%s has reached its connection limit. Autosetup denied.", servername );
         imc_shutdown( FALSE );
         return;
      }
      if( !strcasecmp( txt, "ban" ) )
      {
         imclog( "%s has banned your connection. Autosetup denied.", servername );
         imc_shutdown( FALSE );
         return;
      }
      imclog( "%s: Invalid 'reject' response. Autosetup failed.", servername );
      imclog( "Data received: %s %s %s %s %s", source, servername, cmd, txt, encrypt );
      imc_shutdown( FALSE );
      return;
   }

   if( !strcasecmp( cmd, "accept" ) )
   {
      imclog( "Autosetup completed successfully." );
      if( encrypt && encrypt[0] != '\0' && !strcasecmp( encrypt, "SHA256-SET" ) )
      {
         imclog( "SHA-256 Authentication has been enabled." );
         this_imcmud->sha256pass = TRUE;
         imc_save_config(  );
      }
      imc_finalize_connection( servername, txt );
      return;
   }

   imclog( "%s: Invalid autosetup response.", servername );
   imclog( "Data received: %s %s %s %s %s", source, servername, cmd, txt, encrypt );
   imc_shutdown( FALSE );
   return;
}

bool imc_write_socket( void )
{
   const char *ptr = this_imcmud->outbuf;
   int nleft = this_imcmud->outtop, nwritten = 0;

   if( nleft <= 0 )
      return 1;

   while( nleft > 0 )
   {
      if( ( nwritten = send( this_imcmud->desc, ptr, nleft, 0 ) ) <= 0 )
      {
         if( nwritten == -1 && errno == EAGAIN )
         {
            char *p2 = this_imcmud->outbuf;

            ptr += nwritten;

            while( *ptr != '\0' )
               *p2++ = *ptr++;

            *p2 = '\0';

            this_imcmud->outtop = strlen( this_imcmud->outbuf );
            return TRUE;
         }

         if( nwritten < 0 )
            imclog( "Write error on socket: %s", strerror( errno ) );
         else
            imclog( "%s", "Connection close detected on socket write." );

         imc_shutdown( TRUE );
         return FALSE;
      }
      nleft -= nwritten;
      ptr += nwritten;
   }

   if( imcpacketdebug )
   {
      imclog( "Packet Sent: %s", this_imcmud->outbuf );
      imclog( "Bytes sent: %d", this_imcmud->outtop );
   }
   this_imcmud->outbuf[0] = '\0';
   this_imcmud->outtop = 0;
   return 1;
}

void imc_process_authentication( char *packet )
{
   char command[SMST], rname[SMST], pw[SMST], version[SMST], netname[SMST], encrypt[SMST];
   char response[LGST];

   packet = imcone_argument( packet, command );
   packet = imcone_argument( packet, rname );
   packet = imcone_argument( packet, pw );
   packet = imcone_argument( packet, version ); /* This is more or less ignored */
   packet = imcone_argument( packet, netname );
   packet = imcone_argument( packet, encrypt );

   if( rname[0] == '\0' )
   {
      imclog( "%s", "Incomplete authentication packet. Unable to connect." );
      imc_shutdown( FALSE );
      return;
   }

   if( !strcasecmp( command, "SHA256-AUTH-INIT" ) )
   {
      char pwd[SMST];
      char *cryptpwd;
      long auth_value = 0;

      if( pw[0] == '\0' )
      {
         imclog( "SHA-256 Authentication failure: No auth_value was returned by %s.", rname );
         imc_shutdown( FALSE );
         return;
      }

      /*
       * Lets encrypt this bastard now!
       */
      auth_value = atol( pw );
      snprintf( pwd, SMST, "%ld%s%s", auth_value, this_imcmud->clientpw, this_imcmud->serverpw );
      cryptpwd = sha256_crypt( pwd );

      snprintf( response, LGST, "SHA256-AUTH-RESP %s %s version=%d", this_imcmud->localname, cryptpwd, IMC_VERSION );
      imc_write_buffer( response );
      return;
   }

   /*
    * SHA-256 response is pretty simple. If you blew the authentication, it happened on the server anyway.
    * * rname=servername pw=Networkname
    */
   if( !strcasecmp( command, "SHA256-AUTH-APPR" ) )
   {
      imclog( "%s", "SHA-256 Authentication completed." );
      imc_finalize_connection( rname, pw );
      return;
   }

   /*
    * The old way. Nice and icky, but still very much required for compatibility.
    */
   if( !strcasecmp( command, "PW" ) )
   {
      if( strcasecmp( this_imcmud->serverpw, pw ) )
      {
         imclog( "%s sent an improper serverpassword.", rname );
         imc_shutdown( FALSE );
         return;
      }

      imclog( "%s", "Standard Authentication completed." );
      if( encrypt[0] != '\0' && !strcasecmp( encrypt, "SHA256-SET" ) )
      {
         imclog( "SHA-256 Authentication has been enabled." );
         this_imcmud->sha256pass = TRUE;
         imc_save_config(  );
      }
      imc_finalize_connection( rname, netname );
      return;
   }

   /*
    * Should only be received from servers supporting this obviously
    * * arg1=autosetup name=servername pw=command version=response netname=SHA256-SET
    */
   if( !strcasecmp( command, "autosetup" ) )
   {
      imc_handle_autosetup( command, rname, pw, version, netname );
      return;
   }

   imclog( "Invalid authentication response received from %s!!", rname );
   imclog( "Data received: %s %s %s %s %s", command, rname, pw, version, netname );
   imc_shutdown( FALSE );
   return;
}

/*
 * Transfer one line from input buffer to input line.
 */
bool imc_read_buffer( void )
{
   unsigned int i = 0, j = 0;
   unsigned char ended = 0;
   int k = 0;

   if( this_imcmud->inbuf[0] == '\0' )
      return 0;

   k = strlen( this_imcmud->incomm );

   if( k < 0 )
      k = 0;

   for( i = 0; this_imcmud->inbuf[i] != '\0'
        && this_imcmud->inbuf[i] != '\n' && this_imcmud->inbuf[i] != '\r' && i < IMC_BUFF_SIZE; i++ )
   {
      this_imcmud->incomm[k++] = this_imcmud->inbuf[i];
   }

   while( this_imcmud->inbuf[i] == '\n' || this_imcmud->inbuf[i] == '\r' )
   {
      ended = 1;
      i++;
   }

   this_imcmud->incomm[k] = '\0';

   while( ( this_imcmud->inbuf[j] = this_imcmud->inbuf[i + j] ) != '\0' )
      j++;

   this_imcmud->inbuf[j] = '\0';
   return ended;
}

bool imc_read_socket( void )
{
   unsigned int iStart, iErr;
   bool begin = 1;

   iStart = strlen( this_imcmud->inbuf );

   for( ;; )
   {
      int nRead;

      nRead = recv( this_imcmud->desc, this_imcmud->inbuf + iStart, sizeof( this_imcmud->inbuf ) - 10 - iStart, 0 );
      iErr = errno;
      if( nRead > 0 )
      {
         iStart += nRead;

         if( iStart >= sizeof( this_imcmud->inbuf ) - 10 )
            break;

         begin = 0;
      }
      else if( nRead == 0 && this_imcmud->state == IMC_ONLINE )
      {
         if( !begin )
            break;

         imclog( "%s", "Connection close detected on read of IMC2 socket." );
         return FALSE;
      }
      else if( iErr == EAGAIN )
         break;
      else if( nRead == -1 )
      {
         imclog( "%s: Descriptor error on #%d: %s", __FUNCTION__, this_imcmud->desc, strerror( iErr ) );
         return FALSE;
      }
   }
   this_imcmud->inbuf[iStart] = '\0';
   return TRUE;
}

void imc_loop( void )
{
   fd_set in_set, out_set;
   struct timeval last_time, null_time;

   gettimeofday( &last_time, NULL );
   imc_time = ( time_t ) last_time.tv_sec;

   if( imcwait > 0 )
      imcwait--;

   /*
    * Condition reached only if network shutdown after startup
    */
   if( imcwait == 1 )
   {
      if( ++imcconnect_attempts > 3 )
      {
         if( this_imcmud->sha256pass )
         {
            imclog( "%s", "Unable to reconnect using SHA-256, trying standard authentication." );
            this_imcmud->sha256pass = FALSE;
            imc_save_config();
            imcconnect_attempts = 0;
         }
         else
         {
            imcwait = -2;
            imclog( "%s", "Unable to reestablish connection to server. Abandoning reconnect." );
            return;
         }
      }
      imc_startup( TRUE, -1, FALSE );
      return;
   }

   if( this_imcmud->state == IMC_OFFLINE || this_imcmud->desc == -1 )
      return;

   /*
    * Will prune the cache once every 24hrs after bootup time
    */
   if( imcucache_clock <= imc_time )
   {
      imcucache_clock = imc_time + 86400;
      imc_prune_ucache(  );
   }

   FD_ZERO( &in_set );
   FD_ZERO( &out_set );
   FD_SET( this_imcmud->desc, &in_set );
   FD_SET( this_imcmud->desc, &out_set );

   null_time.tv_sec = null_time.tv_usec = 0;

   if( select( this_imcmud->desc + 1, &in_set, &out_set, NULL, &null_time ) < 0 )
   {
      perror( "imc_loop: select: poll" );
      imc_shutdown( TRUE );
      return;
   }

   if( FD_ISSET( this_imcmud->desc, &in_set ) )
   {
      if( !imc_read_socket( ) )
      {
         if( this_imcmud->inbuf && this_imcmud->inbuf[0] != '\0' )
         {
            if( imc_read_buffer( ) )
            {
               if( !strcasecmp( this_imcmud->incomm, "SHA-256 authentication is required." ) )
               {
                  imclog( "%s", "Unable to reconnect using standard authentication, trying SHA-256." );
                  this_imcmud->sha256pass = TRUE;
                  imc_save_config();
               }
               else
                  imclog( "Buffer contents: %s", this_imcmud->incomm );
            }
         }
         FD_CLR( this_imcmud->desc, &out_set );
         imc_shutdown( TRUE );
         return;
      }

      while( imc_read_buffer(  ) )
      {
         if( imcpacketdebug )
            imclog( "Packet received: %s", this_imcmud->incomm );

         switch ( this_imcmud->state )
         {
            default:
            case IMC_OFFLINE:
            case IMC_AUTH1:  /* Auth1 can only be set when still trying to contact the server */
               break;

            case IMC_AUTH2:  /* Now you've contacted the server and need to process the authentication response */
               imc_process_authentication( this_imcmud->incomm );
               this_imcmud->incomm[0] = '\0';
               break;

            case IMC_ONLINE: /* You're up, pass the bastard off to the packet parser */
               imc_parse_packet( this_imcmud->incomm );
               this_imcmud->incomm[0] = '\0';
               break;
         }
      }
   }

   if( this_imcmud->desc > 0 && this_imcmud->outtop > 0 && FD_ISSET( this_imcmud->desc, &out_set ) && !imc_write_socket(  ) )
   {
      this_imcmud->outtop = 0;
      imc_shutdown( TRUE );
   }
   return;
}

/************************************
 * User login and logout functions. *
 ************************************/

void imc_adjust_perms( CHAR_DATA * ch )
{
   if( !this_imcmud )
      return;

   /*
    * Ugly hack to let the permission system adapt freely, but retains the ability to override that adaptation
    * * in the event you need to restrict someone to a lower level, or grant someone a higher level. This of
    * * course comes at the cost of forgetting you may have done so and caused the override flag to be set, but hey.
    * * This isn't a perfect system and never will be. Samson 2-8-04.
    */
   if( !IMCIS_SET( IMCFLAG( ch ), IMC_PERMOVERRIDE ) )
   {
      if( CH_IMCLEVEL( ch ) < this_imcmud->minlevel )
         IMCPERM( ch ) = IMCPERM_NONE;
      else if( CH_IMCLEVEL( ch ) >= this_imcmud->minlevel && CH_IMCLEVEL( ch ) < this_imcmud->immlevel )
         IMCPERM( ch ) = IMCPERM_MORT;
      else if( CH_IMCLEVEL( ch ) >= this_imcmud->immlevel && CH_IMCLEVEL( ch ) < this_imcmud->adminlevel )
         IMCPERM( ch ) = IMCPERM_IMM;
      else if( CH_IMCLEVEL( ch ) >= this_imcmud->adminlevel && CH_IMCLEVEL( ch ) < this_imcmud->implevel )
         IMCPERM( ch ) = IMCPERM_ADMIN;
      else if( CH_IMCLEVEL( ch ) >= this_imcmud->implevel )
         IMCPERM( ch ) = IMCPERM_IMP;
   }
   return;
}

void imc_char_login( CHAR_DATA * ch )
{
   char buf[SMST];
   int gender, sex;

   if( !this_imcmud )
      return;

   imc_adjust_perms( ch );

   if( this_imcmud->state != IMC_ONLINE )
   {
      if( IMCPERM( ch ) >= IMCPERM_IMM && imcwait == -2 )
         imc_to_char( "~RThe IMC2 connection is down. Attempts to reconnect were abandoned due to excessive failures.\r\n",
                      ch );
      return;
   }

   if( IMCPERM( ch ) < IMCPERM_MORT )
      return;

   snprintf( buf, SMST, "%s@%s", CH_IMCNAME( ch ), this_imcmud->localname );
   gender = imc_get_ucache_gender( buf );
   sex = dikutoimcgender( CH_IMCSEX( ch ) );

   if( gender == sex )
      return;

   imc_ucache_update( buf, sex );
   if( !IMCIS_SET( IMCFLAG( ch ), IMC_INVIS ) )
      imc_send_ucache_update( CH_IMCNAME( ch ), sex );

   return;
}

bool imc_loadchar( CHAR_DATA * ch, FILE * fp, const char *word )
{
   bool fMatch = FALSE;

#if !defined(IMCSTANDALONE)
   if( IS_NPC( ch ) )
      return FALSE;
#endif

   if( IMCPERM( ch ) == IMCPERM_NOTSET )
      imc_adjust_perms( ch );

   switch ( word[0] )
   {
      case 'I':
         IMCKEY( "IMCPerm", IMCPERM( ch ), imcfread_number( fp ) );
         IMCKEY( "IMCEmail", IMC_EMAIL( ch ), imcfread_line( fp ) );
         IMCKEY( "IMCAIM", IMC_AIM( ch ), imcfread_line( fp ) );
         IMCKEY( "IMCICQ", IMC_ICQ( ch ), imcfread_number( fp ) );
         IMCKEY( "IMCYahoo", IMC_YAHOO( ch ), imcfread_line( fp ) );
         IMCKEY( "IMCMSN", IMC_MSN( ch ), imcfread_line( fp ) );
         IMCKEY( "IMCHomepage", IMC_HOMEPAGE( ch ), imcfread_line( fp ) );
         IMCKEY( "IMCComment", IMC_COMMENT( ch ), imcfread_line( fp ) );
         if( !strcasecmp( word, "IMCFlags" ) )
         {
            IMCFLAG( ch ) = imcfread_number( fp );
            imc_char_login( ch );
            fMatch = TRUE;
            break;
         }

         if( !strcasecmp( word, "IMClisten" ) )
         {
            IMC_LISTEN( ch ) = imcfread_line( fp );
            if( IMC_LISTEN( ch ) != NULL && this_imcmud->state == IMC_ONLINE )
            {
               IMC_CHANNEL *channel = NULL;
               char *channels = IMC_LISTEN( ch );
               char arg[SMST];

               while( 1 )
               {
                  if( channels[0] == '\0' )
                     break;
                  channels = imcone_argument( channels, arg );

                  if( !( channel = imc_findchannel( arg ) ) )
                     imc_removename( &IMC_LISTEN( ch ), arg );
                  if( channel && IMCPERM( ch ) < channel->level )
                     imc_removename( &IMC_LISTEN( ch ), arg );
                  if( imc_hasname( IMC_LISTEN( ch ), arg ) )
                     imc_sendnotify( ch, arg, TRUE );
               }
            }
            fMatch = TRUE;
            break;
         }

         if( !strcasecmp( word, "IMCdeny" ) )
         {
            IMC_DENY( ch ) = imcfread_line( fp );
            if( IMC_DENY( ch ) != NULL && this_imcmud->state == IMC_ONLINE )
            {
               IMC_CHANNEL *channel = NULL;
               char *channels = IMC_DENY( ch );
               char arg[SMST];

               while( 1 )
               {
                  if( channels[0] == '\0' )
                     break;
                  channels = imcone_argument( channels, arg );

                  if( !( channel = imc_findchannel( arg ) ) )
                     imc_removename( &IMC_DENY( ch ), arg );
                  if( channel && IMCPERM( ch ) < channel->level )
                     imc_removename( &IMC_DENY( ch ), arg );
               }
            }
            fMatch = TRUE;
            break;
         }

         if( !strcasecmp( word, "IMCignore" ) )
         {
            IMC_IGNORE *temp;

            IMCCREATE( temp, IMC_IGNORE, 1 );
            temp->name = imcfread_line( fp );
            IMCLINK( temp, FIRST_IMCIGNORE( ch ), LAST_IMCIGNORE( ch ), next, prev );
            fMatch = TRUE;
            break;
         }
         break;
   }
   return fMatch;
}

void imc_savechar( CHAR_DATA * ch, FILE * fp )
{
   IMC_IGNORE *temp;

#if !defined(IMCSTANDALONE)
   if( IS_NPC( ch ) )
      return;
#endif

   fprintf( fp, "IMCPerm      %d\n", IMCPERM( ch ) );
   fprintf( fp, "IMCFlags     %ld\n", ( long int )IMCFLAG( ch ) );
   if( IMC_LISTEN( ch ) && IMC_LISTEN( ch )[0] != '\0' )
      fprintf( fp, "IMCListen    %s\n", IMC_LISTEN( ch ) );
   if( IMC_DENY( ch ) && IMC_DENY( ch )[0] != '\0' )
      fprintf( fp, "IMCDeny      %s\n", IMC_DENY( ch ) );
   if( IMC_EMAIL( ch ) && IMC_EMAIL( ch )[0] != '\0' )
      fprintf( fp, "IMCEmail     %s\n", IMC_EMAIL( ch ) );
   if( IMC_HOMEPAGE( ch ) && IMC_HOMEPAGE( ch )[0] != '\0' )
      fprintf( fp, "IMCHomepage  %s\n", IMC_HOMEPAGE( ch ) );
   if( IMC_ICQ( ch ) )
      fprintf( fp, "IMCICQ       %d\n", IMC_ICQ( ch ) );
   if( IMC_AIM( ch ) && IMC_AIM( ch )[0] != '\0' )
      fprintf( fp, "IMCAIM       %s\n", IMC_AIM( ch ) );
   if( IMC_YAHOO( ch ) && IMC_YAHOO( ch )[0] != '\0' )
      fprintf( fp, "IMCYahoo     %s\n", IMC_YAHOO( ch ) );
   if( IMC_MSN( ch ) && IMC_MSN( ch )[0] != '\0' )
      fprintf( fp, "IMCMSN       %s\n", IMC_MSN( ch ) );
   if( IMC_COMMENT( ch ) && IMC_COMMENT( ch )[0] != '\0' )
      fprintf( fp, "IMCComment   %s\n", IMC_COMMENT( ch ) );
   for( temp = FIRST_IMCIGNORE( ch ); temp; temp = temp->next )
      fprintf( fp, "IMCignore    %s\n", temp->name );
   return;
}

#if defined(_DISKIO_H_)
/* This is used only by CircleMUDs which have the ASCII Pfile code installed */
void imc_load_pfile( CHAR_DATA * ch, char *tag, int num, char *line )
{
   if( !strcmp( tag, "IMCPrm" ) )
      IMCPERM( ch ) = num;
   if( !strcmp( tag, "IMCEml" ) )
      IMC_EMAIL( ch ) = IMCSTRALLOC( line );
   if( !strcmp( tag, "IMCAIM" ) )
      IMC_AIM( ch ) = IMCSTRALLOC( line );
   if( !strcmp( tag, "IMCICQ" ) )
      IMC_ICQ( ch ) = num;
   if( !strcmp( tag, "IMCYah" ) )
      IMC_YAHOO( ch ) = IMCSTRALLOC( line );
   if( !strcmp( tag, "IMCMSN" ) )
      IMC_MSN( ch ) = IMCSTRALLOC( line );
   if( !strcmp( tag, "IMCURL" ) )
      IMC_HOMEPAGE( ch ) = IMCSTRALLOC( line );
   if( !strcmp( tag, "IMCCMT" ) )
      IMC_COMMENT( ch ) = IMCSTRALLOC( line );
   if( !strcmp( tag, "IMCFLG" ) )
      IMCFLAG( ch ) = num;
   if( !strcmp( tag, "IMCLSN" ) )
   {
      IMC_LISTEN( ch ) = IMCSTRALLOC( line );
      if( IMC_LISTEN( ch ) != NULL && imc_active == IA_UP )
      {
         IMC_CHANNEL *channel = NULL;
         char *channels = IMC_LISTEN( ch );
         char arg[SMST];

         while( 1 )
         {
            if( channels[0] == '\0' )
               break;
            channels = imcone_argument( channels, arg );

            if( !( channel = imc_findlchannel( arg ) ) )
               imc_removename( &IMC_LISTEN( ch ), arg );
            if( channel && IMCPERM( ch ) < channel->level )
               imc_removename( &IMC_LISTEN( ch ), arg );
            if( imc_hasname( IMC_LISTEN( ch ), arg ) )
               imc_sendnotify( ch, arg, TRUE );
         }
      }
   }

   if( !strcmp( tag, "IMCDNY" ) )
   {
      IMC_DENY( ch ) = IMCSTRALLOC( line );
      if( IMC_DENY( ch ) != NULL && imc_active == IA_UP )
      {
         IMC_CHANNEL *channel = NULL;
         char *channels = IMC_DENY( ch );
         char arg[SMST];

         while( 1 )
         {
            if( channels[0] == '\0' )
               break;
            channels = imcone_argument( channels, arg );

            if( !( channel = imc_findlchannel( arg ) ) )
               imc_removename( &IMC_DENY( ch ), arg );
            if( channel && IMCPERM( ch ) < channel->level )
               imc_removename( &IMC_DENY( ch ), arg );
         }
      }
   }

   if( !strcmp( tag, "IMCIGN" ) )
   {
      IMC_IGNORE *temp;

      IMCCREATE( temp, IMC_IGNORE, 1 );
      temp->name = IMCSTRALLOC( line );
      IMCLINK( temp, FIRST_IMCIGNORE( ch ), LAST_IMCIGNORE( ch ), next, prev );
   }
}

/* This is used only by CircleMUDs which have the ASCII Pfile code installed */
void imc_save_pfile( struct CHAR_DATA *ch, FBFILE * fp )
{
   IMC_IGNORE *temp;

   if( IS_NPC( ch ) )
      return;

   fbprintf( fp, "IMCPrm      %d\n", IMCPERM( ch ) );
   fbprintf( fp, "IMCFLG     %d\n", IMCFLAG( ch ) );
   if( IMC_LISTEN( ch ) && IMC_LISTEN( ch )[0] != '\0' )
      fbprintf( fp, "IMCLSN %s\n", IMC_LISTEN( ch ) );
   if( IMC_DENY( ch ) && IMC_DENY( ch )[0] != '\0' )
      fbprintf( fp, "IMCDNY   %s\n", IMC_DENY( ch ) );
   if( IMC_EMAIL( ch ) && IMC_EMAIL( ch )[0] != '\0' )
      fbprintf( fp, "IMCEml   %s\n", IMC_EMAIL( ch ) );
   if( IMC_HOMEPAGE( ch ) && IMC_HOMEPAGE( ch )[0] != '\0' )
      fbprintf( fp, "IMCURL   %s\n", IMC_HOMEPAGE( ch ) );
   if( IMC_ICQ( ch ) )
      fbprintf( fp, "IMCICQ   %d\n", IMC_ICQ( ch ) );
   if( IMC_AIM( ch ) && IMC_AIM( ch )[0] != '\0' )
      fbprintf( fp, "IMCAIM   %s\n", IMC_AIM( ch ) );
   if( IMC_YAHOO( ch ) && IMC_YAHOO( ch )[0] != '\0' )
      fbprintf( fp, "IMCYah   %s\n", IMC_YAHOO( ch ) );
   if( IMC_MSN( ch ) && IMC_MSN( ch )[0] != '\0' )
      fbprintf( fp, "IMCMSN   %s\n", IMC_MSN( ch ) );
   if( IMC_COMMENT( ch ) && IMC_COMMENT( ch )[0] != '\0' )
      fbprintf( fp, "IMCCMT   %s\n", IMC_COMMENT( ch ) );
   for( temp = FIRST_IMCIGNORE( ch ); temp; temp = temp->next )
      fbprintf( fp, "IMCIGN   %s\n", temp->name );
   return;
}
#endif

void imc_freechardata( CHAR_DATA * ch )
{
   IMC_IGNORE *ign, *ign_next;
   int x;

#if !defined(IMCSTANDALONE)
   if( IS_NPC( ch ) )
      return;
#endif

   if( CH_IMCDATA( ch ) == NULL )
      return;

   for( ign = FIRST_IMCIGNORE( ch ); ign; ign = ign_next )
   {
      ign_next = ign->next;
      IMCSTRFREE( ign->name );
      IMCUNLINK( ign, FIRST_IMCIGNORE( ch ), LAST_IMCIGNORE( ch ), next, prev );
      IMCDISPOSE( ign );
   }
   for( x = 0; x < MAX_IMCTELLHISTORY; x++ )
      IMCSTRFREE( IMCTELLHISTORY( ch, x ) );
   IMCSTRFREE( IMC_LISTEN( ch ) );
   IMCSTRFREE( IMC_DENY( ch ) );
   IMCSTRFREE( IMC_RREPLY( ch ) );
   IMCSTRFREE( IMC_RREPLY_NAME( ch ) );
   IMCSTRFREE( IMC_EMAIL( ch ) );
   IMCSTRFREE( IMC_HOMEPAGE( ch ) );
   IMCSTRFREE( IMC_AIM( ch ) );
   IMCSTRFREE( IMC_YAHOO( ch ) );
   IMCSTRFREE( IMC_MSN( ch ) );
   IMCSTRFREE( IMC_COMMENT( ch ) );
   IMCDISPOSE( CH_IMCDATA( ch ) );
   return;
}

void imc_initchar( CHAR_DATA * ch )
{
#if !defined(IMCSTANDALONE)
   if( IS_NPC( ch ) )
      return;
#endif

   IMCCREATE( CH_IMCDATA( ch ), IMC_CHARDATA, 1 );
   IMC_LISTEN( ch ) = NULL;
   IMC_DENY( ch ) = NULL;
   IMC_RREPLY( ch ) = NULL;
   IMC_RREPLY_NAME( ch ) = NULL;
   IMC_EMAIL( ch ) = NULL;
   IMC_HOMEPAGE( ch ) = NULL;
   IMC_AIM( ch ) = NULL;
   IMC_YAHOO( ch ) = NULL;
   IMC_MSN( ch ) = NULL;
   IMC_COMMENT( ch ) = NULL;
   IMCFLAG( ch ) = 0;
   IMCSET_BIT( IMCFLAG( ch ), IMC_COLORFLAG );
   FIRST_IMCIGNORE( ch ) = NULL;
   LAST_IMCIGNORE( ch ) = NULL;
   IMCPERM( ch ) = IMCPERM_NOTSET;

   return;
}

/*******************************************
 * Network Startup and Shutdown functions. *
 *******************************************/

void imc_loadhistory( void )
{
   char filename[256];
   FILE *tempfile;
   IMC_CHANNEL *tempchan = NULL;
   int x;

   for( tempchan = first_imc_channel; tempchan; tempchan = tempchan->next )
   {
      if( !tempchan->local_name )
         continue;

      snprintf( filename, 256, "%s%s.hist", IMC_DIR, tempchan->local_name );

      if( !( tempfile = fopen( filename, "r" ) ) )
         continue;

      for( x = 0; x < MAX_IMCHISTORY; x++ )
      {
         if( feof( tempfile ) )
            tempchan->history[x] = NULL;
         else
            tempchan->history[x] = imcfread_line( tempfile );
      }
      IMCFCLOSE( tempfile );
      unlink( filename );
   }
}

void imc_savehistory( void )
{
   char filename[256];
   FILE *tempfile;
   IMC_CHANNEL *tempchan = NULL;
   int x;

   for( tempchan = first_imc_channel; tempchan; tempchan = tempchan->next )
   {
      if( !tempchan->local_name )
         continue;

      if( !tempchan->history[0] )
         continue;

      snprintf( filename, 256, "%s%s.hist", IMC_DIR, tempchan->local_name );

      if( !( tempfile = fopen( filename, "w" ) ) )
         continue;

      for( x = 0; x < MAX_IMCHISTORY; x++ )
      {
         if( tempchan->history[x] != NULL )
            fprintf( tempfile, "%s\n", tempchan->history[x] );
      }
      IMCFCLOSE( tempfile );
   }
}

void imc_save_channels( void )
{
   IMC_CHANNEL *c;
   FILE *fp;

   if( !( fp = fopen( IMC_CHANNEL_FILE, "w" ) ) )
   {
      imcbug( "Can't write to %s", IMC_CHANNEL_FILE );
      return;
   }

   for( c = first_imc_channel; c; c = c->next )
   {
      if( !c->local_name || c->local_name[0] == '\0' )
         continue;

      fprintf( fp, "%s", "#IMCCHAN\n" );
      fprintf( fp, "ChanName   %s\n", c->name );
      fprintf( fp, "ChanLocal  %s\n", c->local_name );
      fprintf( fp, "ChanRegF   %s\n", c->regformat );
      fprintf( fp, "ChanEmoF   %s\n", c->emoteformat );
      fprintf( fp, "ChanSocF   %s\n", c->socformat );
      fprintf( fp, "ChanLevel  %d\n", c->level );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   IMCFCLOSE( fp );
}

void imc_readchannel( IMC_CHANNEL * channel, FILE * fp )
{
   const char *word;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : imcfread_word( fp );
      fMatch = FALSE;

      switch ( word[0] )
      {
         case '*':
            fMatch = TRUE;
            imcfread_to_eol( fp );
            break;

         case 'C':
            IMCKEY( "ChanName", channel->name, imcfread_line( fp ) );
            IMCKEY( "ChanLocal", channel->local_name, imcfread_line( fp ) );
            IMCKEY( "ChanRegF", channel->regformat, imcfread_line( fp ) );
            IMCKEY( "ChanEmoF", channel->emoteformat, imcfread_line( fp ) );
            IMCKEY( "ChanSocF", channel->socformat, imcfread_line( fp ) );
            IMCKEY( "ChanLevel", channel->level, imcfread_number( fp ) );
            break;

         case 'E':
            if( !strcasecmp( word, "End" ) )
            {
               /*
                * Legacy support to convert channel permissions
                */
               if( channel->level > IMCPERM_IMP )
               {
                  /*
                   * The IMCPERM_NONE condition should realistically never happen....
                   */
                  if( channel->level < this_imcmud->minlevel )
                     channel->level = IMCPERM_NONE;
                  else if( channel->level >= this_imcmud->minlevel && channel->level < this_imcmud->immlevel )
                     channel->level = IMCPERM_MORT;
                  else if( channel->level >= this_imcmud->immlevel && channel->level < this_imcmud->adminlevel )
                     channel->level = IMCPERM_IMM;
                  else if( channel->level >= this_imcmud->adminlevel && channel->level < this_imcmud->implevel )
                     channel->level = IMCPERM_ADMIN;
                  else if( channel->level >= this_imcmud->implevel )
                     channel->level = IMCPERM_IMP;
               }
            }
            return;
            break;
      }

      if( !fMatch )
         imcbug( "imc_readchannel: no match: %s", word );
   }
}

void imc_loadchannels( void )
{
   FILE *fp;
   IMC_CHANNEL *channel;

   first_imc_channel = NULL;
   last_imc_channel = NULL;

   imclog( "%s", "Loading channels..." );

   if( !( fp = fopen( IMC_CHANNEL_FILE, "r" ) ) )
   {
      imcbug( "%s", "Can't open imc channel file" );
      return;
   }

   for( ;; )
   {
      char letter;
      char *word;

      letter = imcfread_letter( fp );
      if( letter == '*' )
      {
         imcfread_to_eol( fp );
         continue;
      }

      if( letter != '#' )
      {
         imcbug( "%s", "imc_loadchannels: # not found." );
         break;
      }

      word = imcfread_word( fp );
      if( !strcasecmp( word, "IMCCHAN" ) )
      {
         int x;

         IMCCREATE( channel, IMC_CHANNEL, 1 );
         imc_readchannel( channel, fp );

         for( x = 0; x < MAX_IMCHISTORY; x++ )
            channel->history[x] = NULL;

         channel->refreshed = FALSE;   /* Prevents crash trying to use a bogus channel */
         IMCLINK( channel, first_imc_channel, last_imc_channel, next, prev );
         imclog( "configured %s as %s", channel->name, channel->local_name );
         continue;
      }
      else if( !strcasecmp( word, "END" ) )
         break;
      else
      {
         imcbug( "imc_loadchannels: bad section: %s.", word );
         continue;
      }
   }
   IMCFCLOSE( fp );
   return;
}

/* Save current mud-level ban list. Short, simple. */
void imc_savebans( void )
{
   FILE *out;
   IMC_BAN *ban;

   if( !( out = fopen( IMC_BAN_FILE, "w" ) ) )
   {
      imcbug( "%s", "imc_savebans: error opening ban file for write" );
      return;
   }

   fprintf( out, "%s", "#IGNORES\n" );

   for( ban = first_imc_ban; ban; ban = ban->next )
      fprintf( out, "%s\n", ban->name );

   fprintf( out, "%s", "#END\n" );

   IMCFCLOSE( out );
   return;
}

void imc_readbans( void )
{
   FILE *inf;
   char *word;
   char temp[SMST];

   imclog( "%s", "Loading ban list..." );

   if( !( inf = fopen( IMC_BAN_FILE, "r" ) ) )
   {
      imcbug( "%s", "imc_readbans: couldn't open ban file" );
      return;
   }

   word = imcfread_word( inf );
   if( strcasecmp( word, "#IGNORES" ) )
   {
      imcbug( "%s", "imc_readbans: Corrupt file" );
      IMCFCLOSE( inf );
      return;
   }

   while( !feof( inf ) && !ferror( inf ) )
   {
      imcstrlcpy( temp, imcfread_word( inf ), SMST );
      if( !strcasecmp( temp, "#END" ) )
      {
         IMCFCLOSE( inf );
         return;
      }
      imc_addban( temp );
   }

   if( ferror( inf ) )
   {
      perror( "imc_readbans" );
      IMCFCLOSE( inf );
      return;
   }

   IMCFCLOSE( inf );
   return;
}

void imc_savecolor( void )
{
   FILE *fp;
   IMC_COLOR *color;

   if( !( fp = fopen( IMC_COLOR_FILE, "w" ) ) )
   {
      imclog( "%s", "Couldn't write to IMC2 color file." );
      return;
   }

   for( color = first_imc_color; color; color = color->next )
   {
      fprintf( fp, "%s", "#COLOR\n" );
      fprintf( fp, "Name   %s\n", color->name );
      fprintf( fp, "Mudtag %s\n", color->mudtag );
      fprintf( fp, "IMCtag %s\n", color->imctag );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   IMCFCLOSE( fp );
   return;
}

void imc_readcolor( IMC_COLOR * color, FILE * fp )
{
   const char *word;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : imcfread_word( fp );
      fMatch = FALSE;

      switch ( word[0] )
      {
         case '*':
            fMatch = TRUE;
            imcfread_to_eol( fp );
            break;

         case 'E':
            if( !strcasecmp( word, "End" ) )
               return;
            break;

         case 'I':
            IMCKEY( "IMCtag", color->imctag, imcfread_line( fp ) );
            break;

         case 'M':
            IMCKEY( "Mudtag", color->mudtag, imcfread_line( fp ) );
            break;

         case 'N':
            IMCKEY( "Name", color->name, imcfread_line( fp ) );
            break;
      }
      if( !fMatch )
         imcbug( "imc_readcolor: no match: %s", word );
   }
}

void imc_load_color_table( void )
{
   FILE *fp;
   IMC_COLOR *color;

   first_imc_color = last_imc_color = NULL;

   imclog( "%s", "Loading IMC2 color table..." );

   if( !( fp = fopen( IMC_COLOR_FILE, "r" ) ) )
   {
      imclog( "%s", "No color table found." );
      return;
   }

   for( ;; )
   {
      char letter;
      char *word;

      letter = imcfread_letter( fp );
      if( letter == '*' )
      {
         imcfread_to_eol( fp );
         continue;
      }

      if( letter != '#' )
      {
         imcbug( "%s", "imc_load_color_table: # not found." );
         break;
      }

      word = imcfread_word( fp );
      if( !strcasecmp( word, "COLOR" ) )
      {
         IMCCREATE( color, IMC_COLOR, 1 );
         imc_readcolor( color, fp );
         IMCLINK( color, first_imc_color, last_imc_color, next, prev );
         continue;
      }
      else if( !strcasecmp( word, "END" ) )
         break;
      else
      {
         imcbug( "imc_load_color_table: bad section: %s.", word );
         continue;
      }
   }
   IMCFCLOSE( fp );
   return;
}

void imc_savehelps( void )
{
   FILE *fp;
   IMC_HELP_DATA *help;

   if( !( fp = fopen( IMC_HELP_FILE, "w" ) ) )
   {
      imclog( "%s", "Couldn't write to IMC2 help file." );
      return;
   }

   for( help = first_imc_help; help; help = help->next )
   {
      fprintf( fp, "%s", "#HELP\n" );
      fprintf( fp, "Name %s\n", help->name );
      fprintf( fp, "Perm %s\n", imcperm_names[help->level] );
      fprintf( fp, "Text %s¢\n", help->text );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   IMCFCLOSE( fp );
   return;
}

void imc_readhelp( IMC_HELP_DATA * help, FILE * fp )
{
   const char *word;
   char hbuf[LGST];
   int permvalue;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : imcfread_word( fp );
      fMatch = FALSE;

      switch ( word[0] )
      {
         case '*':
            fMatch = TRUE;
            imcfread_to_eol( fp );
            break;

         case 'E':
            if( !strcasecmp( word, "End" ) )
               return;
            break;

         case 'N':
            IMCKEY( "Name", help->name, imcfread_line( fp ) );
            break;

         case 'P':
            if( !strcasecmp( word, "Perm" ) )
            {
               word = imcfread_word( fp );
               permvalue = get_imcpermvalue( word );

               if( permvalue < 0 || permvalue > IMCPERM_IMP )
               {
                  imcbug( "imc_readhelp: Command %s loaded with invalid permission. Set to Imp.", help->name );
                  help->level = IMCPERM_IMP;
               }
               else
                  help->level = permvalue;
               fMatch = TRUE;
               break;
            }
            break;

         case 'T':
            if( !strcasecmp( word, "Text" ) )
            {
               int num = 0;

               while( ( hbuf[num] = fgetc( fp ) ) != EOF && hbuf[num] != '¢' && num < ( LGST - 2 ) )
                  num++;
               hbuf[num] = '\0';
               help->text = IMCSTRALLOC( hbuf );
               fMatch = TRUE;
               break;
            }
            break;
      }
      if( !fMatch )
         imcbug( "imc_readhelp: no match: %s", word );
   }
}

void imc_load_helps( void )
{
   FILE *fp;
   IMC_HELP_DATA *help;

   first_imc_help = last_imc_help = NULL;

   imclog( "%s", "Loading IMC2 help file..." );

   if( !( fp = fopen( IMC_HELP_FILE, "r" ) ) )
   {
      imclog( "%s", "No help file found." );
      return;
   }

   for( ;; )
   {
      char letter;
      char *word;

      letter = imcfread_letter( fp );
      if( letter == '*' )
      {
         imcfread_to_eol( fp );
         continue;
      }

      if( letter != '#' )
      {
         imcbug( "%s", "imc_load_helps: # not found." );
         break;
      }

      word = imcfread_word( fp );
      if( !strcasecmp( word, "HELP" ) )
      {
         IMCCREATE( help, IMC_HELP_DATA, 1 );
         imc_readhelp( help, fp );
         IMCLINK( help, first_imc_help, last_imc_help, next, prev );
         continue;
      }
      else if( !strcasecmp( word, "END" ) )
         break;
      else
      {
         imcbug( "imc_load_helps: bad section: %s.", word );
         continue;
      }
   }
   IMCFCLOSE( fp );
   return;
}

void imc_savecommands( void )
{
   FILE *fp;
   IMC_CMD_DATA *cmd;
   IMC_ALIAS *alias;

   if( !( fp = fopen( IMC_CMD_FILE, "w" ) ) )
   {
      imclog( "%s", "Couldn't write to IMC2 command file." );
      return;
   }

   for( cmd = first_imc_command; cmd; cmd = cmd->next )
   {
      fprintf( fp, "%s", "#COMMAND\n" );
      fprintf( fp, "Name      %s\n", cmd->name );
      if( cmd->function != NULL )
         fprintf( fp, "Code      %s\n", imc_funcname( cmd->function ) );
      else
         fprintf( fp, "%s", "Code      NULL\n" );
      fprintf( fp, "Perm      %s\n", imcperm_names[cmd->level] );
      fprintf( fp, "Connected %d\n", cmd->connected );
      for( alias = cmd->first_alias; alias; alias = alias->next )
         fprintf( fp, "Alias     %s\n", alias->name );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   IMCFCLOSE( fp );
   return;
}

void imc_readcommand( IMC_CMD_DATA * cmd, FILE * fp )
{
   IMC_ALIAS *alias;
   const char *word;
   int permvalue;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : imcfread_word( fp );
      fMatch = FALSE;

      switch ( word[0] )
      {
         case '*':
            fMatch = TRUE;
            imcfread_to_eol( fp );
            break;

         case 'E':
            if( !strcasecmp( word, "End" ) )
               return;
            break;

         case 'A':
            if( !strcasecmp( word, "Alias" ) )
            {
               IMCCREATE( alias, IMC_ALIAS, 1 );
               alias->name = imcfread_line( fp );
               IMCLINK( alias, cmd->first_alias, cmd->last_alias, next, prev );
               fMatch = TRUE;
               break;
            }
            break;

         case 'C':
            IMCKEY( "Connected", cmd->connected, imcfread_number( fp ) );
            if( !strcasecmp( word, "Code" ) )
            {
               word = imcfread_word( fp );
               cmd->function = imc_function( word );
               if( cmd->function == NULL )
                  imcbug( "imc_readcommand: Command %s loaded with invalid function. Set to NULL.", cmd->name );
               fMatch = TRUE;
               break;
            }
            break;

         case 'N':
            IMCKEY( "Name", cmd->name, imcfread_line( fp ) );
            break;

         case 'P':
            if( !strcasecmp( word, "Perm" ) )
            {
               word = imcfread_word( fp );
               permvalue = get_imcpermvalue( word );

               if( permvalue < 0 || permvalue > IMCPERM_IMP )
               {
                  imcbug( "imc_readcommand: Command %s loaded with invalid permission. Set to Imp.", cmd->name );
                  cmd->level = IMCPERM_IMP;
               }
               else
                  cmd->level = permvalue;
               fMatch = TRUE;
               break;
            }
            break;
      }
      if( !fMatch )
         imcbug( "imc_readcommand: no match: %s", word );
   }
}

bool imc_load_commands( void )
{
   FILE *fp;
   IMC_CMD_DATA *cmd;

   first_imc_command = last_imc_command = NULL;

   imclog( "%s", "Loading IMC2 command table..." );

   if( !( fp = fopen( IMC_CMD_FILE, "r" ) ) )
   {
      imclog( "%s", "No command table found." );
      return FALSE;
   }

   for( ;; )
   {
      char letter;
      char *word;

      letter = imcfread_letter( fp );
      if( letter == '*' )
      {
         imcfread_to_eol( fp );
         continue;
      }

      if( letter != '#' )
      {
         imcbug( "%s", "imc_load_commands: # not found." );
         break;
      }

      word = imcfread_word( fp );
      if( !strcasecmp( word, "COMMAND" ) )
      {
         IMCCREATE( cmd, IMC_CMD_DATA, 1 );
         imc_readcommand( cmd, fp );
         IMCLINK( cmd, first_imc_command, last_imc_command, next, prev );
         continue;
      }
      else if( !strcasecmp( word, "END" ) )
         break;
      else
      {
         imcbug( "imc_load_commands: bad section: %s.", word );
         continue;
      }
   }
   IMCFCLOSE( fp );
   return TRUE;
}

void imc_readucache( IMCUCACHE_DATA * user, FILE * fp )
{
   const char *word;
   bool fMatch;

   for( ;; )
   {
      word = feof( fp ) ? "End" : imcfread_word( fp );
      fMatch = FALSE;

      switch ( word[0] )
      {
         case '*':
            fMatch = TRUE;
            imcfread_to_eol( fp );
            break;

         case 'N':
            IMCKEY( "Name", user->name, imcfread_line( fp ) );
            break;

         case 'S':
            IMCKEY( "Sex", user->gender, imcfread_number( fp ) );
            break;

         case 'T':
            IMCKEY( "Time", user->time, imcfread_number( fp ) );
            break;

         case 'E':
            if( !strcasecmp( word, "End" ) )
               return;
            break;
      }
      if( !fMatch )
         imcbug( "imc_readucache: no match: %s", word );
   }
}

void imc_load_ucache( void )
{
   FILE *fp;
   IMCUCACHE_DATA *user;

   imclog( "%s", "Loading ucache data..." );

   if( !( fp = fopen( IMC_UCACHE_FILE, "r" ) ) )
   {
      imclog( "%s", "No ucache data found." );
      return;
   }

   for( ;; )
   {
      char letter;
      char *word;

      letter = imcfread_letter( fp );
      if( letter == '*' )
      {
         imcfread_to_eol( fp );
         continue;
      }

      if( letter != '#' )
      {
         imcbug( "%s", "imc_load_ucahe: # not found." );
         break;
      }

      word = imcfread_word( fp );
      if( !strcasecmp( word, "UCACHE" ) )
      {
         IMCCREATE( user, IMCUCACHE_DATA, 1 );
         imc_readucache( user, fp );
         IMCLINK( user, first_imcucache, last_imcucache, next, prev );
         continue;
      }
      else if( !strcasecmp( word, "END" ) )
         break;
      else
      {
         imcbug( "imc_load_ucache: bad section: %s.", word );
         continue;
      }
   }
   IMCFCLOSE( fp );
   imc_prune_ucache(  );
   imcucache_clock = imc_time + 86400;
   return;
}

void imc_save_config( void )
{
   FILE *fp;

   if( !( fp = fopen( IMC_CONFIG_FILE, "w" ) ) )
   {
      imclog( "%s", "Couldn't write to config file." );
      return;
   }

   fprintf( fp, "%s", "$IMCCONFIG\n\n" );
   fprintf( fp, "# %s config file.\n", this_imcmud->versionid );
   fprintf( fp, "%s", "# This file can now support the use of tildes in your strings.\n" );
   fprintf( fp, "%s", "# This information can be edited online using the 'imcconfig' command.\n" );
   fprintf( fp, "LocalName      %s\n", this_imcmud->localname );
   fprintf( fp, "Autoconnect    %d\n", this_imcmud->autoconnect );
   fprintf( fp, "MinPlayerLevel %d\n", this_imcmud->minlevel );
   fprintf( fp, "MinImmLevel    %d\n", this_imcmud->immlevel );
   fprintf( fp, "AdminLevel     %d\n", this_imcmud->adminlevel );
   fprintf( fp, "Implevel       %d\n", this_imcmud->implevel );
   fprintf( fp, "InfoName       %s\n", this_imcmud->fullname );
   fprintf( fp, "InfoHost       %s\n", this_imcmud->ihost );
   fprintf( fp, "InfoPort       %d\n", this_imcmud->iport );
   fprintf( fp, "InfoEmail      %s\n", this_imcmud->email );
   fprintf( fp, "InfoWWW        %s\n", this_imcmud->www );
   fprintf( fp, "InfoBase       %s\n", this_imcmud->base );
   fprintf( fp, "InfoDetails    %s\n\n", this_imcmud->details );
   fprintf( fp, "%s", "# Your server connection information goes here.\n" );
   fprintf( fp, "%s", "# This information should be available from the network you plan to join.\n" );
   fprintf( fp, "ServerAddr     %s\n", this_imcmud->rhost );
   fprintf( fp, "ServerPort     %d\n", this_imcmud->rport );
   fprintf( fp, "ClientPwd      %s\n", this_imcmud->clientpw );
   fprintf( fp, "ServerPwd      %s\n", this_imcmud->serverpw );
   fprintf( fp, "#SHA256 auth: 0 = disabled, 1 = enabled\n" );
   fprintf( fp, "SHA256         %d\n", this_imcmud->sha256 );
   if( this_imcmud->sha256pass )
   {
      fprintf( fp, "%s",
               "#Your server is expecting SHA256 authentication now. Do not remove this line unless told to do so.\n" );
      fprintf( fp, "SHA256Pwd      %d\n", this_imcmud->sha256pass );
   }
   fprintf( fp, "%s", "End\n\n" );
   fprintf( fp, "%s", "$END\n" );
   IMCFCLOSE( fp );
   return;
}

void imcfread_config_file( FILE * fin )
{
   const char *word;
   bool fMatch;

   for( ;; )
   {
      word = feof( fin ) ? "end" : imcfread_word( fin );
      fMatch = FALSE;

      switch ( word[0] )
      {
         case '#':
            fMatch = TRUE;
            imcfread_to_eol( fin );
            break;

         case 'A':
            IMCKEY( "Autoconnect", this_imcmud->autoconnect, imcfread_number( fin ) );
            IMCKEY( "AdminLevel", this_imcmud->adminlevel, imcfread_number( fin ) );
            break;

         case 'C':
            IMCKEY( "ClientPwd", this_imcmud->clientpw, imcfread_line( fin ) );
            break;

         case 'E':
            if( !strcasecmp( word, "End" ) )
            {
#if defined(IMCCHRONICLES)
               char lbuf1[LGST], lbuf2[LGST];

               snprintf( lbuf1, LGST, "%s %s.%s", CODEBASE_VERSION_TITLE, CODEBASE_VERSION_MAJOR, CODEBASE_VERSION_MINOR );
               if( this_imcmud->base )
                  IMCSTRFREE( this_imcmud->base );
               this_imcmud->base = IMCSTRALLOC( lbuf1 );

               snprintf( lbuf2, LGST, "%s%s", IMC_VERSION_STRING, this_imcmud->base );
               this_imcmud->versionid = IMCSTRALLOC( lbuf2 );
#endif
               return;
            }
            break;

         case 'I':
            IMCKEY( "Implevel", this_imcmud->implevel, imcfread_number( fin ) );
            IMCKEY( "InfoName", this_imcmud->fullname, imcfread_line( fin ) );
            IMCKEY( "InfoHost", this_imcmud->ihost, imcfread_line( fin ) );
            IMCKEY( "InfoPort", this_imcmud->iport, imcfread_number( fin ) );
            IMCKEY( "InfoEmail", this_imcmud->email, imcfread_line( fin ) );
            IMCKEY( "InfoWWW", this_imcmud->www, imcfread_line( fin ) );
            IMCKEY( "InfoBase", this_imcmud->base, imcfread_line( fin ) );
            IMCKEY( "InfoDetails", this_imcmud->details, imcfread_line( fin ) );
            break;

         case 'L':
            IMCKEY( "LocalName", this_imcmud->localname, imcfread_line( fin ) );
            break;

         case 'M':
            IMCKEY( "MinImmLevel", this_imcmud->immlevel, imcfread_number( fin ) );
            IMCKEY( "MinPlayerLevel", this_imcmud->minlevel, imcfread_number( fin ) );
            break;

         case 'R':
            IMCKEY( "RouterAddr", this_imcmud->rhost, imcfread_line( fin ) );
            IMCKEY( "RouterPort", this_imcmud->rport, imcfread_number( fin ) );
            break;

         case 'S':
            IMCKEY( "ServerPwd", this_imcmud->serverpw, imcfread_line( fin ) );
            IMCKEY( "ServerAddr", this_imcmud->rhost, imcfread_line( fin ) );
            IMCKEY( "ServerPort", this_imcmud->rport, imcfread_number( fin ) );
            IMCKEY( "SHA256", this_imcmud->sha256, imcfread_number( fin ) );
            IMCKEY( "SHA256Pwd", this_imcmud->sha256pass, imcfread_number( fin ) );
            break;
      }
      if( !fMatch )
         imcbug( "%s: Bad keyword: %s\r\n", __FUNCTION__, word );
   }
}

bool imc_read_config( int desc )
{
   FILE *fin;
   char cbase[SMST];

   if( this_imcmud != NULL )
      imc_delete_info(  );
   this_imcmud = NULL;

   imclog( "%s", "Loading IMC2 network data..." );

   if( !( fin = fopen( IMC_CONFIG_FILE, "r" ) ) )
   {
      imclog( "%s", "Can't open configuration file" );
      imclog( "%s", "Network configuration aborted." );
      return FALSE;
   }

   for( ;; )
   {
      char letter;
      char *word;

      letter = imcfread_letter( fin );

      if( letter == '#' )
      {
         imcfread_to_eol( fin );
         continue;
      }

      if( letter != '$' )
      {
         imcbug( "%s", "imc_read_config: $ not found" );
         break;
      }

      word = imcfread_word( fin );
      if( !strcasecmp( word, "IMCCONFIG" ) && this_imcmud == NULL )
      {
         IMCCREATE( this_imcmud, SITEINFO, 1 );

         /*
          * If someone can think of better default values, I'm all ears. Until then, keep your bitching to yourselves.
          */
         this_imcmud->minlevel = 10;
         this_imcmud->immlevel = 101;
         this_imcmud->adminlevel = 113;
         this_imcmud->implevel = 115;
         this_imcmud->network = IMCSTRALLOC( "Unknown" );
         this_imcmud->sha256 = TRUE;
         this_imcmud->sha256pass = FALSE;
         this_imcmud->desc = desc;

         imcfread_config_file( fin );
         continue;
      }
      else if( !strcasecmp( word, "END" ) )
         break;
      else
      {
         imcbug( "imc_read_config: Bad section in config file: %s", word );
         continue;
      }
   }
   IMCFCLOSE( fin );

   if( !this_imcmud )
   {
      imcbug( "%s", "imc_read_config: No server connection information!!" );
      imcbug( "%s", "Network configuration aborted." );
      return FALSE;
   }

   if( !this_imcmud->rhost || !this_imcmud->clientpw || !this_imcmud->serverpw )
   {
      imcbug( "%s", "imc_read_config: Missing required configuration info." );
      imcbug( "%s", "Network configuration aborted." );
      return FALSE;
   }

   if( !this_imcmud->localname || this_imcmud->localname[0] == '\0' )
   {
      imcbug( "%s", "imc_read_config: Mud name not loaded in configuration file." );
      imcbug( "%s", "Network configuration aborted." );
      return FALSE;
   }

   if( !this_imcmud->fullname || this_imcmud->fullname[0] == '\0' )
   {
      imcbug( "%s", "imc_read_config: Missing InfoName parameter in configuration file." );
      imcbug( "%s", "Network configuration aborted." );
      return FALSE;
   }

   if( !this_imcmud->ihost || this_imcmud->ihost[0] == '\0' )
   {
      imcbug( "%s", "imc_read_config: Missing InfoHost parameter in configuration file." );
      imcbug( "%s", "Network configuration aborted." );
      return FALSE;
   }

   if( !this_imcmud->email || this_imcmud->email[0] == '\0' )
   {
      imcbug( "%s", "imc_read_config: Missing InfoEmail parameter in configuration file." );
      imcbug( "%s", "Network configuration aborted." );
      return FALSE;
   }

   if( !this_imcmud->base || this_imcmud->base[0] == '\0' )
      this_imcmud->base = IMCSTRALLOC( "Unknown Codebase" );

   if( !this_imcmud->www || this_imcmud->www[0] == '\0' )
      this_imcmud->www = IMCSTRALLOC( "Not specified" );

   if( !this_imcmud->details || this_imcmud->details[0] == '\0' )
      this_imcmud->details = IMCSTRALLOC( "No details provided." );

   if( !this_imcmud->versionid )
   {
      snprintf( cbase, SMST, "%s%s", IMC_VERSION_STRING, this_imcmud->base );
      this_imcmud->versionid = IMCSTRALLOC( cbase );
   }
   return TRUE;
}

char *parse_who_header( char *head )
{
   static char newhead[LGST];
   char iport[SMST];

   snprintf( iport, SMST, "%d", this_imcmud->iport );
   imcstrlcpy( newhead, head, LGST );
   imcstrlcpy( newhead, imcstrrep( newhead, "<%mudfullname%>", this_imcmud->fullname ), LGST );
   imcstrlcpy( newhead, imcstrrep( newhead, "<%mudtelnet%>", this_imcmud->ihost ), LGST );
   imcstrlcpy( newhead, imcstrrep( newhead, "<%mudport%>", iport ), LGST );
   imcstrlcpy( newhead, imcstrrep( newhead, "<%mudurl%>", this_imcmud->www ), LGST );

   return newhead;
}

char *parse_who_tail( char *tail )
{
   static char newtail[LGST];
   char iport[SMST];

   snprintf( iport, SMST, "%d", this_imcmud->iport );
   imcstrlcpy( newtail, tail, LGST );
   imcstrlcpy( newtail, imcstrrep( newtail, "<%mudfullname%>", this_imcmud->fullname ), LGST );
   imcstrlcpy( newtail, imcstrrep( newtail, "<%mudtelnet%>", this_imcmud->ihost ), LGST );
   imcstrlcpy( newtail, imcstrrep( newtail, "<%mudport%>", iport ), LGST );
   imcstrlcpy( newtail, imcstrrep( newtail, "<%mudurl%>", this_imcmud->www ), LGST );

   return newtail;
}

void imc_delete_who_template( void )
{
   IMCSTRFREE( whot->head );
   IMCSTRFREE( whot->plrheader );
   IMCSTRFREE( whot->immheader );
   IMCSTRFREE( whot->plrline );
   IMCSTRFREE( whot->immline );
   IMCSTRFREE( whot->tail );
   IMCSTRFREE( whot->master );
   IMCDISPOSE( whot );
}

void imc_load_who_template( void )
{
   FILE *fp;
   char hbuf[LGST];
   char *word;
   int num;

   imclog( "%s", "Loading IMC2 who template..." );

   if( !( fp = fopen( IMC_WHO_FILE, "r" ) ) )
   {
      imclog( "%s: Unable to load template file for imcwho", __FUNCTION__ );
      whot = NULL;
      return;
   }

   if( whot )
      imc_delete_who_template();
   IMCCREATE( whot, WHO_TEMPLATE, 1 );

   do
   {
      word = imcfread_word( fp );
      hbuf[0] = '\0';
      num = 0;

      if( !strcasecmp( word, "Head:" ) )
      {
         while( ( hbuf[num] = fgetc( fp ) ) != EOF && hbuf[num] != '¢' && num < ( LGST - 2 ) )
            ++num;
         hbuf[num] = '\0';
         whot->head = IMCSTRALLOC( parse_who_header( hbuf ) );
      }
      else if( !strcasecmp( word, "Tail:" ) )
      {
         while( ( hbuf[num] = fgetc( fp ) ) != EOF && hbuf[num] != '¢' && num < ( LGST - 2 ) )
            ++num;
         hbuf[num] = '\0';
         whot->tail = IMCSTRALLOC( parse_who_tail( hbuf ) );
      }
      else if( !strcasecmp( word, "Plrline:" ) )
      {
         while( ( hbuf[num] = fgetc( fp ) ) != EOF && hbuf[num] != '¢' && num < ( LGST - 2 ) )
            ++num;
         hbuf[num] = '\0';
         whot->plrline = IMCSTRALLOC( hbuf );
      }
      else if( !strcasecmp( word, "Immline:" ) )
      {
         while( ( hbuf[num] = fgetc( fp ) ) != EOF && hbuf[num] != '¢' && num < ( LGST - 2 ) )
            ++num;
         hbuf[num] = '\0';
         whot->immline = IMCSTRALLOC( hbuf );
      }
      else if( !strcasecmp( word, "Immheader:" ) )
      {
         while( ( hbuf[num] = fgetc( fp ) ) != EOF && hbuf[num] != '¢' && num < ( LGST - 2 ) )
            ++num;
         hbuf[num] = '\0';
         whot->immheader = IMCSTRALLOC( hbuf );
      }
      else if( !strcasecmp( word, "Plrheader:" ) )
      {
         while( ( hbuf[num] = fgetc( fp ) ) != EOF && hbuf[num] != '¢' && num < ( LGST - 2 ) )
            ++num;
         hbuf[num] = '\0';
         whot->plrheader = IMCSTRALLOC( hbuf );
      }
      else if( !strcasecmp( word, "Master:" ) )
      {
         while( ( hbuf[num] = fgetc( fp ) ) != EOF && hbuf[num] != '¢' && num < ( LGST - 2 ) )
            ++num;
         hbuf[num] = '\0';
         whot->master = IMCSTRALLOC( hbuf );
      }
   }
   while( !feof( fp ) );
   IMCFCLOSE( fp );
}

void imc_load_templates( void )
{
   imc_load_who_template();
}

int ipv4_connect( void )
{
   struct sockaddr_in sa;
   struct hostent *hostp;
#ifdef WIN32
   ULONG r;
#else
   int r;
#endif
   int desc = -1;

   memset( &sa, 0, sizeof( sa ) );
   sa.sin_family = AF_INET;

#ifndef WIN32
   /*
    * warning: this blocks. It would be better to farm the query out to
    * * another process, but that is difficult to do without lots of changes
    * * to the core mud code. You may want to change this code if you have an
    * * existing resolver process running.
    */
   if( !inet_aton( this_imcmud->rhost, &sa.sin_addr ) )
   {
      hostp = gethostbyname( this_imcmud->rhost );
      if( !hostp )
      {
         imclog( "%s", "imc_connect_to: Cannot resolve server hostname." );
         imc_shutdown( FALSE );
         return -1;
      }
      memcpy( &sa.sin_addr, hostp->h_addr, hostp->h_length );
   }
#else
   sa.sin_addr.s_addr = inet_addr(this_imcmud->rhost);
#endif

   sa.sin_port = htons( this_imcmud->rport );

   desc = socket( AF_INET, SOCK_STREAM, 0 );
   if( desc < 0 )
   {
      perror( "socket" );
      return -1;
   }

#ifdef WIN32
   r = 1;
   if( ioctlsocket( desc, FIONBIO, &r ) == SOCKET_ERROR )
   {
      perror( "imc_connect: ioctlsocket failed" );
      close( desc );
      return;
   }
#else
   r = fcntl( desc, F_GETFL, 0 );
   if( r < 0 || fcntl( desc, F_SETFL, O_NONBLOCK | r ) < 0 )
   {
      perror( "imc_connect: fcntl" );
      close( desc );
      return -1;
   }
#endif

   if( connect( desc, ( struct sockaddr * )&sa, sizeof( sa ) ) == -1 )
   {
      if( errno != EINPROGRESS )
      {
         imclog( "%s: Failed connect: Error %d: %s", __FUNCTION__, errno, strerror( errno ) );
         perror( "connect" );
         close( desc );
         return -1;
      }
   }
   return desc;
}

bool imc_server_connect( void )
{
#if defined(IPV6)
   struct addrinfo hints, *ai_list, *ai;
   char rport[SMST];
   int n, r;
#endif
   char buf[LGST];
   int desc = 0;

   if( !this_imcmud )
   {
      imcbug( "%s", "No connection data loaded" );
      return FALSE;
   }

   if( this_imcmud->state != IMC_AUTH1 )
   {
      imcbug( "%s", "Connection is not in proper state." );
      return FALSE;
   }

   if( this_imcmud->desc > 0 )
   {
      imcbug( "%s", "Already connected" );
      return FALSE;
   }

#if defined(IPV6)
   snprintf( rport, SMST, "%hu", this_imcmud->rport );
   memset( &hints, 0, sizeof( struct addrinfo ) );
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = IPPROTO_TCP;
   n = getaddrinfo( this_imcmud->rhost, rport, &hints, &ai_list );

   if( n )
   {
      imclog( "%s: getaddrinfo: %s", __FUNCTION__, gai_strerror( n ) );
      return FALSE;
   }

   for( ai = ai_list; ai; ai = ai->ai_next )
   {
      desc = socket( ai->ai_family, ai->ai_socktype, ai->ai_protocol );
      if( desc < 0 )
         continue;

      if( connect( desc, ai->ai_addr, ai->ai_addrlen ) == 0 )
         break;
      close( desc );
   }
   freeaddrinfo( ai_list );
   if( ai == NULL )
   {
      imclog( "%s: socket or connect: failed for %s port %hu", __FUNCTION__, this_imcmud->rhost, this_imcmud->rport );
      imcwait = 100; /* So it will try again according to the reconnect count. */
      return FALSE;
   }

   r = fcntl( desc, F_GETFL, 0 );
   if( r < 0 || fcntl( desc, F_SETFL, O_NONBLOCK | r ) < 0 )
   {
      perror( "imc_connect: fcntl" );
      close( desc );
      return FALSE;
   }
#else
   desc = ipv4_connect(  );
   if( desc < 1 )
      return FALSE;
#endif

   imclog( "%s", "Connecting to server." );

   this_imcmud->state = IMC_AUTH2;
   this_imcmud->desc = desc;
   this_imcmud->inbuf[0] = '\0';
   this_imcmud->outsize = 1000;
   IMCCREATE( this_imcmud->outbuf, char, this_imcmud->outsize );

   /*
    * The MUD is electing to enable SHA256 - this is the default setting
    */
   if( this_imcmud->sha256 )
   {
      /*
       * No SHA256 setup enabled.
       * * Situations where this might happen:
       * *
       * * 1. You are connecting for the first time. This is expected.
       * * 2. You are connecting to an older server which does not support it, so you will continue connecting this way.
       * * 3. You got stupid and deleted the SHA256 line in your config file after it got there. Ooops.
       * * 4. The server lost your data. In which case you'll need to do #3 because authentication will fail.
       * * 5. You let your connection lapse, and #4 happened because of it.
       * * 6. Gremlins. When in doubt, blame them.
       */
      if( !this_imcmud->sha256pass )
      {
         snprintf( buf, LGST, "PW %s %s version=%d autosetup %s SHA256",
                   this_imcmud->localname, this_imcmud->clientpw, IMC_VERSION, this_imcmud->serverpw );
      }
      /*
       * You have SHA256 working. Excellent. Lets send the new packet for it.
       * * Situations where this will fail:
       * *
       * * 1. You're a new connection, and for whatever dumb reason, the SHA256 line is in your config already.
       * * 2. You have SHA256 enabled and you're switching to a new server. This is generally not going to work well.
       * * 3. Something happened and the hashing failed. Resulting in authentication failure. Ooops.
       * * 4. The server lost your connection data.
       * * 5. You let your connection lapse, and #4 happened because of it.
       * * 6. Gremlins. When in doubt, blame them.
       */
      else
         snprintf( buf, LGST, "SHA256-AUTH-REQ %s", this_imcmud->localname );
   }
   /*
    * The MUD is electing not to use SHA256 for whatever reason - this must be specifically set
    */
   else
      snprintf( buf, LGST, "PW %s %s version=%d autosetup %s",
                this_imcmud->localname, this_imcmud->clientpw, IMC_VERSION, this_imcmud->serverpw );

   imc_write_buffer( buf );
   return TRUE;
}

void imc_delete_templates( void )
{
   imc_delete_who_template();
}

void free_imcdata( bool complete )
{
   REMOTEINFO *p, *pnext;
   IMC_BAN *ban, *ban_next;
   IMCUCACHE_DATA *ucache, *next_ucache;
   IMC_CMD_DATA *cmd, *cmd_next;
   IMC_ALIAS *alias, *alias_next;
   IMC_HELP_DATA *help, *help_next;
   IMC_COLOR *color, *color_next;
   IMC_PHANDLER *ph, *ph_next;
   IMC_CHANNEL *c, *c_next;

   for( c = first_imc_channel; c; c = c_next )
   {
      c_next = c->next;
      imc_freechan( c );
   }

   for( p = first_rinfo; p; p = pnext )
   {
      pnext = p->next;
      imc_delete_reminfo( p );
   }

   for( ban = first_imc_ban; ban; ban = ban_next )
   {
      ban_next = ban->next;
      imc_freeban( ban );
   }

   for( ucache = first_imcucache; ucache; ucache = next_ucache )
   {
      next_ucache = ucache->next;
      IMCSTRFREE( ucache->name );
      IMCUNLINK( ucache, first_imcucache, last_imcucache, next, prev );
      IMCDISPOSE( ucache );
   }

   /*
    * This stuff is only killed off if the mud itself shuts down. For those of you Valgrinders out there.
    */
   if( complete )
   {
      imc_delete_templates();

      for( cmd = first_imc_command; cmd; cmd = cmd_next )
      {
         cmd_next = cmd->next;

         for( alias = cmd->first_alias; alias; alias = alias_next )
         {
            alias_next = alias->next;

            IMCSTRFREE( alias->name );
            IMCUNLINK( alias, cmd->first_alias, cmd->last_alias, next, prev );
            IMCDISPOSE( alias );
         }
         IMCSTRFREE( cmd->name );
         IMCUNLINK( cmd, first_imc_command, last_imc_command, next, prev );
         IMCDISPOSE( cmd );
      }

      for( help = first_imc_help; help; help = help_next )
      {
         help_next = help->next;
         IMCSTRFREE( help->name );
         IMCSTRFREE( help->text );
         IMCUNLINK( help, first_imc_help, last_imc_help, next, prev );
         IMCDISPOSE( help );
      }

      for( color = first_imc_color; color; color = color_next )
      {
         color_next = color->next;
         IMCSTRFREE( color->name );
         IMCSTRFREE( color->mudtag );
         IMCSTRFREE( color->imctag );
         IMCUNLINK( color, first_imc_color, last_imc_color, next, prev );
         IMCDISPOSE( color );
      }

      for( ph = first_phandler; ph; ph = ph_next )
      {
         ph_next = ph->next;

         IMCSTRFREE( ph->name );
         IMCUNLINK( ph, first_phandler, last_phandler, next, prev );
         IMCDISPOSE( ph );
      }
   }
   return;
}

void imc_hotboot( void )
{
   FILE *fp;

   if( this_imcmud && this_imcmud->state == IMC_ONLINE )
   {
      if( !( fp = fopen( IMC_HOTBOOT_FILE, "w" ) ) )
         imcbug( "%s: Unable to open IMC hotboot file for write.", __FUNCTION__ );
      else
      {
         fprintf( fp, "%s %s\n", ( this_imcmud->network ? this_imcmud->network : "Unknown" ),
                  ( this_imcmud->servername ? this_imcmud->servername : "Unknown" ) );
         IMCFCLOSE( fp );
         imc_savehistory(  );
      }
   }
}

/* Shutdown IMC2 */
void imc_shutdown( bool reconnect )
{
   if( this_imcmud && this_imcmud->state == IMC_OFFLINE )
      return;

   imclog( "%s", "Shutting down network." );

   if( this_imcmud->desc > 0 )
      close( this_imcmud->desc );
   this_imcmud->desc = -1;

   imc_savehistory(  );
   free_imcdata( FALSE );

   this_imcmud->state = IMC_OFFLINE;
   if( reconnect )
   {
      imcwait = 100; /* About 20 seconds or so */
      imclog( "%s", "Connection to server was lost. Reconnecting in approximately 20 seconds." );
   }
}

/* Startup IMC2 */
bool imc_startup_network( bool connected )
{
   imclog( "%s", "IMC2 Network Initializing..." );

   if( connected )
   {
      FILE *fp;
      char netname[SMST], server[SMST];

      if( !( fp = fopen( IMC_HOTBOOT_FILE, "r" ) ) )
         imcbug( "%s: Unable to load IMC hotboot file.", __FUNCTION__ );
      else
      {
         unlink( IMC_HOTBOOT_FILE );

         int errorcheck = fscanf( fp, "%s %s\n", netname, server );
         errorcheck = 0; // no purpose of variable, only to avoid warning (void) type casting doesn't work

         IMCSTRFREE( this_imcmud->network );
         this_imcmud->network = IMCSTRALLOC( netname );
         IMCSTRFREE( this_imcmud->servername );
         this_imcmud->servername = IMCSTRALLOC( server );
         IMCFCLOSE( fp );
      }
      this_imcmud->state = IMC_ONLINE;
      this_imcmud->inbuf[0] = '\0';
      this_imcmud->outsize = IMC_BUFF_SIZE;
      IMCCREATE( this_imcmud->outbuf, char, this_imcmud->outsize );
      imc_request_keepalive(  );
      imc_firstrefresh(  );
      return TRUE;
   }

   this_imcmud->state = IMC_AUTH1;

   /*
    * Connect to server
    */
   if( !imc_server_connect(  ) )
   {
      this_imcmud->state = IMC_OFFLINE;
      return FALSE;
   }

   return TRUE;
}

void imc_startup( bool force, int desc, bool connected )
{
   imcwait = 0;

   if( this_imcmud && this_imcmud->state > IMC_OFFLINE )
   {
      imclog( "%s: Network startup called when already engaged!", __FUNCTION__ );
      return;
   }

   imc_time = time( NULL );
   imc_sequencenumber = imc_time;

   /*
    * The Command table is required for operation. Like.... duh?
    */
   if( first_imc_command == NULL )
   {
      if( !imc_load_commands(  ) )
      {
         imcbug( "%s: Unable to load command table!", __FUNCTION__ );
         return;
      }
   }

   /*
    * Configuration required for network operation.
    */
   if( !imc_read_config( desc ) )
      return;

   /*
    * Lets register all the default packet handlers
    */
   imc_register_default_packets(  );

   /*
    * Help information should persist even when the network is not connected...
    */
   if( first_imc_help == NULL )
      imc_load_helps(  );

   /*
    * ... as should the color table.
    */
   if( first_imc_color == NULL )
      imc_load_color_table(  );

   /*
    * ... and the templates. Checks for whot being defined, but the others are loaded here to, so....
    */
   if( !whot )
      imc_load_templates();

   if( ( !this_imcmud->autoconnect && !force && !connected ) || ( connected && this_imcmud->desc < 1 ) )
   {
      imclog( "%s", "IMC2 network data loaded. Autoconnect not set. IMC2 will need to be connected manually." );
      return;
   }
   else
      imclog( "%s", "IMC2 network data loaded." );

   if( this_imcmud->autoconnect || force || connected )
   {
      if( imc_startup_network( connected ) )
      {
         imc_loadchannels(  );
         imc_loadhistory(  );
         imc_readbans(  );
         imc_load_ucache(  );
         return;
      }
   }
   return;
}

/*****************************************
 * User level commands and social hooks. *
 *****************************************/

/* The imccommand command, aka icommand. Channel manipulation at the server level etc. */
IMC_CMD( imccommand )
{
   char cmd[SMST], chan[SMST], to[SMST];
   IMC_PACKET *p;
   IMC_CHANNEL *c;

   argument = imcone_argument( argument, cmd );
   argument = imcone_argument( argument, chan );

   if( !cmd[0] || !chan[0] )
   {
      imc_to_char( "Syntax: imccommand <command> <server:channel> [<data..>]\r\n", ch );
      imc_to_char( "Command access will depend on your privledges and what each individual server allows.\r\n", ch );
      return;
   }

   if( !( c = imc_findchannel( chan ) ) && strcasecmp( cmd, "create" ) )
   {
      imc_printf( ch, "There is no channel called %s known.\r\n", chan );
      return;
   }

   snprintf( to, SMST, "IMC@%s", c ? imc_channel_mudof( c->name ) : imc_channel_mudof( chan ) );
   p = imc_newpacket( CH_IMCNAME( ch ), "ice-cmd", to );
   imc_addtopacket( p, "channel=%s", c ? c->name : chan );
   imc_addtopacket( p, "command=%s", cmd );
   if( argument && argument[0] != '\0' )
      imc_addtopacket( p, "data=%s", argument );
   imc_write_packet( p );

   imc_to_char( "Command sent.\r\n", ch );
   return;
}

/* need exactly 2 %s's, and no other format specifiers */
bool verify_format( const char *fmt, short sneed )
{
   const char *c;
   int i = 0;

   c = fmt;
   while( ( c = strchr( c, '%' ) ) != NULL )
   {
      if( *( c + 1 ) == '%' ) /* %% */
      {
         c += 2;
         continue;
      }

      if( *( c + 1 ) != 's' ) /* not %s */
         return FALSE;

      c++;
      i++;
   }
   if( i != sneed )
      return FALSE;

   return TRUE;
}

/* The imcsetup command, channel manipulation at the mud level etc, formatting and the like.
 * This command will not do "localization" since this is handled automatically now by ice-update packets.
 */
IMC_CMD( imcsetup )
{
   char imccmd[SMST], chan[SMST], arg1[SMST], buf[LGST];
   IMC_CHANNEL *c = NULL;
   int x;
   bool all = FALSE;

   argument = imcone_argument( argument, imccmd );
   argument = imcone_argument( argument, chan );
   argument = imcone_argument( argument, arg1 );

   if( imccmd[0] == '\0' || chan[0] == '\0' )
   {
      imc_to_char( "Syntax: imcsetup <command> <channel> [<data..>]\r\n", ch );
      imc_to_char( "Where 'command' is one of the following:\r\n", ch );
      imc_to_char( "delete rename perm regformat emoteformat socformat\r\n\r\n", ch );
      imc_to_char( "Where 'channel' is one of the following:\r\n", ch );
      for( c = first_imc_channel; c; c = c->next )
      {
         if( c->local_name && c->local_name[0] != '\0' )
            imc_printf( ch, "%s ", c->local_name );
         else
            imc_printf( ch, "%s ", c->name );
      }
      imc_to_char( "\r\n", ch );
      return;
   }

   if( !strcasecmp( chan, "all" ) )
      all = TRUE;
   else
   {
      if( !( c = imc_findchannel( chan ) ) )
      {
         imc_to_char( "Unknown channel.\r\n", ch );
         return;
      }
   }

   /*
    * Permission check -- Xorith
    */
   if( c && c->level > IMCPERM( ch ) )
   {
      imc_to_char( "You cannot modify that channel.", ch );
      return;
   }

   if( !strcasecmp( imccmd, "delete" ) )
   {
      if( all )
      {
         imc_to_char( "You cannot perform a delete all on channels.\r\n", ch );
         return;
      }
      IMCSTRFREE( c->local_name );
      IMCSTRFREE( c->regformat );
      IMCSTRFREE( c->emoteformat );
      IMCSTRFREE( c->socformat );

      for( x = 0; x < MAX_IMCHISTORY; x++ )
         IMCSTRFREE( c->history[x] );

      imc_to_char( "Channel is no longer locally configured.\r\n", ch );

      if( !c->refreshed )
         imc_freechan( c );
      imc_save_channels(  );
      return;
   }

   if( !strcasecmp( imccmd, "rename" ) )
   {
      if( all )
      {
         imc_to_char( "You cannot perform a rename all on channels.\r\n", ch );
         return;
      }

      if( arg1[0] == '\0' )
      {
         imc_to_char( "Missing 'newname' argument for 'imcsetup rename'\r\n", ch ); /* Lets be more kind! -- X */
         imc_to_char( "Syntax: imcsetup rename <local channel> <newname>\r\n", ch );   /* Fixed syntax message -- X */
         return;
      }

      if( imc_findchannel( arg1 ) )
      {
         imc_to_char( "New channel name already exists.\r\n", ch );
         return;
      }

      /*
       * Small change here to give better feedback to the ch -- Xorith
       */
      snprintf( buf, LGST, "Renamed channel '%s' to '%s'.\r\n", c->local_name, arg1 );
      IMCSTRFREE( c->local_name );
      c->local_name = IMCSTRALLOC( arg1 );
      imc_to_char( buf, ch );

      /*
       * Reset the format with the new local name
       */
      imcformat_channel( ch, c, 4, FALSE );
      imc_save_channels(  );
      return;
   }

   if( !strcasecmp( imccmd, "resetformats" ) )
   {
      if( all )
      {
         imcformat_channel( ch, NULL, 4, TRUE );
         imc_to_char( "All channel formats have been reset to default.\r\n", ch );
      }
      else
      {
         imcformat_channel( ch, c, 4, FALSE );
         imc_to_char( "The formats for this channel have been reset to default.\r\n", ch );
      }
      return;
   }

   if( !strcasecmp( imccmd, "regformat" ) )
   {
      if( !arg1[0] )
      {
         imc_to_char( "Syntax: imcsetup regformat <localchannel|all> <string>\r\n", ch ); /* Syntax Fix -- Xorith */
         return;
      }

      if( !verify_format( arg1, 2 ) )
      {
         imc_to_char( "Bad format - must contain exactly 2 %s's.\r\n", ch );
         return;
      }

      if( all )
      {
         for( c = first_imc_channel; c; c = c->next )
         {
            IMCSTRFREE( c->regformat );
            c->regformat = IMCSTRALLOC( arg1 );
         }
         imc_to_char( "All channel regular formats have been updated.\r\n", ch );
      }
      else
      {
         IMCSTRFREE( c->regformat );
         c->regformat = IMCSTRALLOC( arg1 );
         imc_to_char( "The regular format for this channel has been changed successfully.\r\n", ch );
      }
      imc_save_channels(  );
      return;
   }

   if( !strcasecmp( imccmd, "emoteformat" ) )
   {
      if( !arg1[0] )
      {
         imc_to_char( "Syntax: imcsetup emoteformat <localchannel|all> <string>\r\n", ch );  /* Syntax Fix -- Xorith */
         return;
      }

      if( !verify_format( arg1, 2 ) )
      {
         imc_to_char( "Bad format - must contain exactly 2 %s's.\r\n", ch );
         return;
      }

      if( all )
      {
         for( c = first_imc_channel; c; c = c->next )
         {
            IMCSTRFREE( c->emoteformat );
            c->emoteformat = IMCSTRALLOC( arg1 );
         }
         imc_to_char( "All channel emote formats have been updated.\r\n", ch );
      }
      else
      {
         IMCSTRFREE( c->emoteformat );
         c->emoteformat = IMCSTRALLOC( arg1 );
         imc_to_char( "The emote format for this channel has been changed successfully.\r\n", ch );
      }
      imc_save_channels(  );
      return;
   }

   if( !strcasecmp( imccmd, "socformat" ) )
   {
      if( !arg1[0] )
      {
         imc_to_char( "Syntax: imcsetup socformat <localchannel|all> <string>\r\n", ch ); /* Xorith */
         return;
      }

      if( !verify_format( arg1, 1 ) )
      {
         imc_to_char( "Bad format - must contain exactly 1 %s.\r\n", ch );
         return;
      }

      if( all )
      {
         for( c = first_imc_channel; c; c = c->next )
         {
            IMCSTRFREE( c->socformat );
            c->socformat = IMCSTRALLOC( arg1 );
         }
         imc_to_char( "All channel social formats have been updated.\r\n", ch );
      }
      else
      {
         IMCSTRFREE( c->socformat );
         c->socformat = IMCSTRALLOC( arg1 );
         imc_to_char( "The social format for this channel has been changed successfully.\r\n", ch );
      }
      imc_save_channels(  );
      return;
   }

   if( !strcasecmp( imccmd, "perm" ) || !strcasecmp( imccmd, "permission" ) || !strcasecmp( imccmd, "level" ) )
   {
      int permvalue = -1;

      if( all )
      {
         imc_to_char( "You cannot do a permissions all for channels.\r\n", ch );
         return;
      }

      if( !arg1[0] )
      {
         imc_to_char( "Syntax: imcsetup perm <localchannel> <permission>\r\n", ch );
         return;
      }

      permvalue = get_imcpermvalue( arg1 );
      if( permvalue < 0 || permvalue > IMCPERM_IMP )
      {
         imc_to_char( "Unacceptable permission setting.\r\n", ch );
         return;
      }

      /*
       * Added permission checking here -- Xorith
       */
      if( permvalue > IMCPERM( ch ) )
      {
         imc_to_char( "You cannot set a permission higher than your own.\r\n", ch );
         return;
      }

      c->level = permvalue;

      imc_to_char( "Channel permissions changed.\r\n", ch );
      imc_save_channels(  );
      return;
   }
   imcsetup( ch, "" );
   return;
}

/* The imcchanlist command. Basic listing of channels. */
IMC_CMD( imcchanlist )
{
   IMC_CHANNEL *c = NULL;
   int count = 0; /* Count -- Xorith */
   char col = 'C';   /* Listening Color -- Xorith */

   if( !first_imc_channel )
   {
      imc_to_char( "~WThere are no known channels on this network.\r\n", ch );
      return;
   }

   if( argument && argument[0] != '\0' )
   {
      if( !( c = imc_findchannel( argument ) ) )
      {
         imc_printf( ch, "There is no channel called %s here.\r\n", argument );
         return;
      }
   }

   if( c )
   {
      imc_printf( ch, "~WChannel  : %s\r\n\r\n", c->name );
      imc_printf( ch, "~cLocalname: ~w%s\r\n", c->local_name );
      imc_printf( ch, "~cPerms    : ~w%s\r\n", imcperm_names[c->level] );
      imc_printf( ch, "~cPolicy   : %s\r\n", c->open ? "~gOpen" : "~yPrivate" );
      imc_printf( ch, "~cRegFormat: ~w%s\r\n", c->regformat );
      imc_printf( ch, "~cEmoFormat: ~w%s\r\n", c->emoteformat );
      imc_printf( ch, "~cSocFormat: ~w%s\r\n\r\n", c->socformat );
      imc_printf( ch, "~cOwner    : ~w%s\r\n", c->owner );
      imc_printf( ch, "~cOperators: ~w%s\r\n", c->operators );
      imc_printf( ch, "~cInvite   : ~w%s\r\n", c->invited );
      imc_printf( ch, "~cExclude  : ~w%s\r\n", c->excluded );
      return;
   }

   imc_printf( ch, "~c%-15s ~C%-15s ~B%-15s ~b%-7s ~!%s\r\n\r\n", "Name", "Local name", "Owner", "Perm", "Policy" );
   for( c = first_imc_channel; c; c = c->next )
   {
      if( IMCPERM( ch ) < c->level )
         continue;

      /*
       * If it's locally configured and we're not listening, then color it red -- Xorith
       */
      if( c->local_name )
      {
         if( !imc_hasname( IMC_LISTEN( ch ), c->local_name ) )
            col = 'R';
         else
            col = 'C';  /* Otherwise, keep it Cyan -- X */
      }

      imc_printf( ch, "~c%-15.15s ~%c%-*.*s ~B%-15.15s ~b%-7s %s\r\n", c->name, col,
                  c->local_name ? 15 : 17, c->local_name ? 15 : 17,
                  c->local_name ? c->local_name : "~Y(not local)  ", c->owner,
                  imcperm_names[c->level], c->refreshed ? ( c->open ? "~gOpen" : "~yPrivate" ) : "~Runknown" );
      count++; /* Keep a count -- Xorith */
   }
   /*
    * Show the count and a bit of text explaining the red color -- Xorith
    */
   imc_printf( ch, "\r\n~W%d ~cchannels found.", count );
   imc_to_char( "\r\n~RRed ~clocal name indicates a channel not being listened to.\r\n", ch );
   return;
}

IMC_CMD( imclisten )
{
   IMC_CHANNEL *c;

   if( !argument || argument[0] == '\0' )
   {
      imc_to_char( "~cCurrently tuned into:\r\n", ch );
      if( IMC_LISTEN( ch ) && IMC_LISTEN( ch )[0] != '\0' )
         imc_printf( ch, "~W%s", IMC_LISTEN( ch ) );
      else
         imc_to_char( "~WNone", ch );
      imc_to_char( "\r\n", ch );
      return;
   }

   if( !strcasecmp( argument, "all" ) )
   {
      for( c = first_imc_channel; c; c = c->next )
      {
         if( !c->local_name )
            continue;

         if( IMCPERM( ch ) >= c->level && !imc_hasname( IMC_LISTEN( ch ), c->local_name ) )
            imc_addname( &IMC_LISTEN( ch ), c->local_name );
         imc_sendnotify( ch, c->local_name, TRUE );
      }
      imc_to_char( "~YYou are now listening to all available IMC2 channels.\r\n", ch );
      return;
   }

   if( !strcasecmp( argument, "none" ) )
   {
      for( c = first_imc_channel; c; c = c->next )
      {
         if( !c->local_name )
            continue;

         if( imc_hasname( IMC_LISTEN( ch ), c->local_name ) )
            imc_removename( &IMC_LISTEN( ch ), c->local_name );
         imc_sendnotify( ch, c->local_name, FALSE );
      }
      imc_to_char( "~YYou no longer listen to any available IMC2 channels.\r\n", ch );
      return;
   }

   if( !( c = imc_findchannel( argument ) ) )
   {
      imc_to_char( "No such channel configured locally.\r\n", ch );
      return;
   }

   if( IMCPERM( ch ) < c->level )
   {
      imc_to_char( "No such channel configured locally.\r\n", ch );
      return;
   }

   if( imc_hasname( IMC_LISTEN( ch ), c->local_name ) )
   {
      imc_removename( &IMC_LISTEN( ch ), c->local_name );
      imc_to_char( "Channel off.\r\n", ch );
      imc_sendnotify( ch, c->local_name, FALSE );
   }
   else
   {
      imc_addname( &IMC_LISTEN( ch ), c->local_name );
      imc_to_char( "Channel on.\r\n", ch );
      imc_sendnotify( ch, c->local_name, TRUE );
   }
}

IMC_CMD( imctell )
{
   char buf[LGST], buf1[LGST];

   if( IMCIS_SET( IMCFLAG( ch ), IMC_DENYTELL ) )
   {
      imc_to_char( "You are not authorized to use imctell.\r\n", ch );
      return;
   }

   argument = imcone_argument( argument, buf );

   if( !argument || argument[0] == '\0' )
   {
      int x;

      imc_to_char( "Usage: imctell user@mud <message>\r\n", ch );
      imc_to_char( "Usage: imctell [on]/[off]\r\n\r\n", ch );
      imc_printf( ch, "&cThe last %d things you were told:\r\n", MAX_IMCTELLHISTORY );

      for( x = 0; x < MAX_IMCTELLHISTORY; x++ )
      {
         if( IMCTELLHISTORY( ch, x ) == NULL )
            break;
         imc_to_char( IMCTELLHISTORY( ch, x ), ch );
      }
      return;
   }

   if( !strcasecmp( argument, "on" ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_TELL );
      imc_to_char( "You now send and receive imctells.\r\n", ch );
      return;
   }

   if( !strcasecmp( argument, "off" ) )
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_TELL );
      imc_to_char( "You no longer send and receive imctells.\r\n", ch );
      return;
   }

   if( IMCIS_SET( IMCFLAG( ch ), IMC_TELL ) )
   {
      imc_to_char( "You have imctells turned off.\r\n", ch );
      return;
   }

   if( IMCISINVIS( ch ) )
   {
      imc_to_char( "You are invisible.\r\n", ch );
      return;
   }

   if( !check_mudof( ch, buf ) )
      return;

   /*
    * Tell socials. Suggested by Darien@Sandstorm
    */
   if( argument[0] == '@' )
   {
      char *p, *p2;
      char buf2[SMST];

      argument++;
      while( isspace( *argument ) )
         argument++;
      imcstrlcpy( buf2, argument, SMST );
      p = imc_send_social( ch, argument, 1 );
      if( !p || p[0] == '\0' )
         return;

      imc_send_tell( CH_IMCNAME( ch ), buf, p, 2 );
      p2 = imc_send_social( ch, buf2, 2 );
      if( !p2 || p2[0] == '\0' )
         return;
      snprintf( buf1, LGST, "~WImctell ~C%s: ~c%s\r\n", buf, p2 );
   }
   else if( argument[0] == ',' )
   {
      argument++;
      while( isspace( *argument ) )
         argument++;
      imc_send_tell( CH_IMCNAME( ch ), buf, color_mtoi( argument ), 1 );
      snprintf( buf1, LGST, "~WImctell: ~c%s %s\r\n", buf, argument );
   }
   else
   {
      imc_send_tell( CH_IMCNAME( ch ), buf, color_mtoi( argument ), 0 );
      snprintf( buf1, LGST, "~cYou imctell ~C%s ~c'~W%s~c'\r\n", buf, argument );
   }
   imc_to_char( buf1, ch );
   imc_update_tellhistory( ch, buf1 );
   return;
}

IMC_CMD( imcreply )
{
   char buf1[LGST];

   /*
    * just check for deny
    */
   if( IMCIS_SET( IMCFLAG( ch ), IMC_DENYTELL ) )
   {
      imc_to_char( "You are not authorized to use imcreply.\r\n", ch );
      return;
   }

   if( IMCIS_SET( IMCFLAG( ch ), IMC_TELL ) )
   {
      imc_to_char( "You have imctells turned off.\r\n", ch );
      return;
   }

   if( IMCISINVIS( ch ) )
   {
      imc_to_char( "You are invisible.\r\n", ch );
      return;
   }

   if( !IMC_RREPLY( ch ) )
   {
      imc_to_char( "You haven't received an imctell yet.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      imc_to_char( "imcreply what?\r\n", ch );
      return;
   }

   if( !check_mudof( ch, IMC_RREPLY( ch ) ) )
      return;

   /*
    * Tell socials. Suggested by Darien@Sandstorm
    */
   if( argument[0] == '@' )
   {
      char *p, *p2;
      char buf2[SMST];

      argument++;
      while( isspace( *argument ) )
         argument++;
      imcstrlcpy( buf2, argument, SMST );
      p = imc_send_social( ch, argument, 1 );
      if( !p || p[0] == '\0' )
         return;

      imc_send_tell( CH_IMCNAME( ch ), IMC_RREPLY( ch ), p, 2 );
      p2 = imc_send_social( ch, buf2, 2 );
      if( !p2 || p2[0] == '\0' )
         return;
      snprintf( buf1, LGST, "~WImctell ~C%s: ~c%s\r\n", IMC_RREPLY( ch ), p2 );
   }
   else if( argument[0] == ',' )
   {
      argument++;
      while( isspace( *argument ) )
         argument++;
      imc_send_tell( CH_IMCNAME( ch ), IMC_RREPLY( ch ), color_mtoi( argument ), 1 );
      snprintf( buf1, LGST, "~WImctell: ~c%s %s\r\n", IMC_RREPLY( ch ), argument );
   }
   else
   {
      imc_send_tell( CH_IMCNAME( ch ), IMC_RREPLY( ch ), color_mtoi( argument ), 0 );
      snprintf( buf1, LGST, "~cYou imctell ~C%s ~c'~W%s~c'\r\n", IMC_RREPLY( ch ), argument );
   }
   imc_to_char( buf1, ch );
   imc_update_tellhistory( ch, buf1 );
   return;
}

IMC_CMD( imcwho )
{
   if( !argument || argument[0] == '\0' )
   {
      imc_to_char( "imcwho which mud? See imclist for a list of connected muds.\r\n", ch );
      return;
   }

   /* Now why didn't I think of this before for local who testing?
    * Meant for testing only, so it needs >= Imm perms
    * Otherwise people could use it to bypass wizinvis locally.
    */
   if( !strcasecmp( argument, this_imcmud->localname ) && IMCPERM(ch) >= IMCPERM_IMM )
   {
      imc_to_char( imc_assemble_who(), ch );
      return;
   }

   if( !check_mud( ch, argument ) )
      return;

   imc_send_who( CH_IMCNAME( ch ), argument, "who" );
}

IMC_CMD( imclocate )
{
   char user[SMST];

   if( !argument || argument[0] == '\0' )
   {
      imc_to_char( "imclocate who?\r\n", ch );
      return;
   }

   snprintf( user, SMST, "%s@*", argument );
   imc_send_whois( CH_IMCNAME( ch ), user );
   return;
}

IMC_CMD( imcfinger )
{
   char name[LGST], arg[SMST];

   if( IMCIS_SET( IMCFLAG( ch ), IMC_DENYFINGER ) )
   {
      imc_to_char( "You are not authorized to use imcfinger.\r\n", ch );
      return;
   }

   argument = imcone_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      imc_to_char( "~wUsage: imcfinger person@mud\r\n", ch );
      imc_to_char( "~wUsage: imcfinger <field> <value>\r\n", ch );
      imc_to_char( "~wWhere field is one of:\r\n\r\n", ch );
      imc_to_char( "~wdisplay email homepage icq aim yahoo msn privacy comment\r\n", ch );
      return;
   }

   if( !strcasecmp( arg, "display" ) )
   {
      imc_to_char( "~GYour current information:\r\n\r\n", ch );
      imc_printf( ch, "~GEmail   : ~g%s\r\n", ( IMC_EMAIL( ch ) && IMC_EMAIL( ch )[0] != '\0' ) ? IMC_EMAIL( ch ) : "None" );
      imc_printf( ch, "~GHomepage: ~g%s\r\n",
                  ( IMC_HOMEPAGE( ch ) && IMC_HOMEPAGE( ch )[0] != '\0' ) ? IMC_HOMEPAGE( ch ) : "None" );
      imc_printf( ch, "~GICQ     : ~g%d\r\n", IMC_ICQ( ch ) );
      imc_printf( ch, "~GAIM     : ~g%s\r\n", ( IMC_AIM( ch ) && IMC_AIM( ch )[0] != '\0' ) ? IMC_AIM( ch ) : "None" );
      imc_printf( ch, "~GYahoo   : ~g%s\r\n", ( IMC_YAHOO( ch ) && IMC_YAHOO( ch )[0] != '\0' ) ? IMC_YAHOO( ch ) : "None" );
      imc_printf( ch, "~GMSN     : ~g%s\r\n", ( IMC_MSN( ch ) && IMC_MSN( ch )[0] != '\0' ) ? IMC_MSN( ch ) : "None" );
      imc_printf( ch, "~GComment : ~g%s\r\n",
                  ( IMC_COMMENT( ch ) && IMC_COMMENT( ch )[0] != '\0' ) ? IMC_COMMENT( ch ) : "None" );
      imc_printf( ch, "~GPrivacy : ~g%s\r\n", IMCIS_SET( IMCFLAG( ch ), IMC_PRIVACY ) ? "Enabled" : "Disabled" );
      return;
   }

   if( !strcasecmp( arg, "privacy" ) )
   {
      if( IMCIS_SET( IMCFLAG( ch ), IMC_PRIVACY ) )
      {
         IMCREMOVE_BIT( IMCFLAG( ch ), IMC_PRIVACY );
         imc_to_char( "Privacy flag removed. Your information will now be visible on imcfinger.\r\n", ch );
      }
      else
      {
         IMCSET_BIT( IMCFLAG( ch ), IMC_PRIVACY );
         imc_to_char( "Privacy flag enabled. Your information will no longer be visible on imcfinger.\r\n", ch );
      }
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      if( this_imcmud->state != IMC_ONLINE )
      {
         imc_to_char( "The mud is not currently connected to IMC2.\r\n", ch );
         return;
      }

      if( !check_mudof( ch, arg ) )
         return;

      snprintf( name, LGST, "finger %s", imc_nameof( arg ) );
      imc_send_who( CH_IMCNAME( ch ), imc_mudof( arg ), name );
      return;
   }

   if( !strcasecmp( arg, "email" ) )
   {
      IMCSTRFREE( IMC_EMAIL( ch ) );
      IMC_EMAIL( ch ) = IMCSTRALLOC( argument );
      imc_printf( ch, "Your email address has changed to: %s\r\n", IMC_EMAIL( ch ) );
      return;
   }

   if( !strcasecmp( arg, "homepage" ) )
   {
      IMCSTRFREE( IMC_HOMEPAGE( ch ) );
      IMC_HOMEPAGE( ch ) = IMCSTRALLOC( argument );
      imc_printf( ch, "Your homepage has changed to: %s\r\n", IMC_HOMEPAGE( ch ) );
      return;
   }

   if( !strcasecmp( arg, "icq" ) )
   {
      IMC_ICQ( ch ) = atoi( argument );
      imc_printf( ch, "Your ICQ Number has changed to: %d\r\n", IMC_ICQ( ch ) );
      return;
   }

   if( !strcasecmp( arg, "aim" ) )
   {
      IMCSTRFREE( IMC_AIM( ch ) );
      IMC_AIM( ch ) = IMCSTRALLOC( argument );
      imc_printf( ch, "Your AIM Screenname has changed to: %s\r\n", IMC_AIM( ch ) );
      return;
   }

   if( !strcasecmp( arg, "yahoo" ) )
   {
      IMCSTRFREE( IMC_YAHOO( ch ) );
      IMC_YAHOO( ch ) = IMCSTRALLOC( argument );
      imc_printf( ch, "Your Yahoo Screenname has changed to: %s\r\n", IMC_YAHOO( ch ) );
      return;
   }

   if( !strcasecmp( arg, "msn" ) )
   {
      IMCSTRFREE( IMC_MSN( ch ) );
      IMC_MSN( ch ) = IMCSTRALLOC( argument );
      imc_printf( ch, "Your MSN Screenname has changed to: %s\r\n", IMC_MSN( ch ) );
      return;
   }

   if( !strcasecmp( arg, "comment" ) )
   {
      if( strlen( argument ) > 78 )
      {
         imc_to_char( "You must limit the comment line to 78 characters or less.\r\n", ch );
         return;
      }
      IMCSTRFREE( IMC_COMMENT( ch ) );
      IMC_COMMENT( ch ) = IMCSTRALLOC( argument );
      imc_printf( ch, "Your comment line has changed to: %s\r\n", IMC_COMMENT( ch ) );
      return;
   }
   imcfinger( ch, "" );
   return;
}

/* Removed imcquery and put in imcinfo. -- Xorith */
IMC_CMD( imcinfo )
{
   if( !argument || argument[0] == '\0' )
   {
      imc_to_char( "Syntax: imcinfo <mud>\r\n", ch );
      return;
   }

   if( !check_mud( ch, argument ) )
      return;

   imc_send_who( CH_IMCNAME( ch ), argument, "info" );
   return;
}

IMC_CMD( imcbeep )
{
   if( IMCIS_SET( IMCFLAG( ch ), IMC_DENYBEEP ) )
   {
      imc_to_char( "You are not authorized to use imcbeep.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      imc_to_char( "Usage: imcbeep user@mud\r\n", ch );
      imc_to_char( "Usage: imcbeep [on]/[off]\r\n", ch );
      return;
   }

   if( !strcasecmp( argument, "on" ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_BEEP );
      imc_to_char( "You now send and receive imcbeeps.\r\n", ch );
      return;
   }

   if( !strcasecmp( argument, "off" ) )
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_BEEP );
      imc_to_char( "You no longer send and receive imcbeeps.\r\n", ch );
      return;
   }

   if( IMCIS_SET( IMCFLAG( ch ), IMC_BEEP ) )
   {
      imc_to_char( "You have imcbeep turned off.\r\n", ch );
      return;
   }

   if( IMCISINVIS( ch ) )
   {
      imc_to_char( "You are invisible.\r\n", ch );
      return;
   }

   if( !check_mudof( ch, argument ) )
      return;

   imc_send_beep( CH_IMCNAME( ch ), argument );
   imc_printf( ch, "~cYou imcbeep ~Y%s~c.\r\n", argument );
   return;
}

#ifdef WEBSVR
void web_imc_list( WEB_DESCRIPTOR *wdesc )
{
   REMOTEINFO *rin;
   char *start, *onpath;
   char buf[MAX_STRING_LENGTH], urldump[MAX_INPUT_LENGTH], netname[MAX_INPUT_LENGTH], serverpath[MAX_INPUT_LENGTH];
   int end, count = 1;

   snprintf( buf, MAX_STRING_LENGTH, "<table><tr><td colspan=\"5\"><font color=\"white\">Active muds on %s:</font></td></tr>",
             this_imcmud->network );
   snprintf( buf + strlen( buf ), MAX_STRING_LENGTH,
             "<tr><td><font color=\"#008080\">%s</font></td><td><font color=\"blue\">%s</font></td><td><font color=\"green\">%s</font></td><td><font color=\"#00FF00\">%s</font></td></tr>",
             "Name", "IMC Version", "Network", "Route" );
   send_buf( wdesc->fd, buf );

   if( !this_imcmud->www || !str_cmp( this_imcmud->www, "??" ) || !str_cmp( this_imcmud->www, "Unknown" )
       || !strstr( this_imcmud->www, "http://" ) )
   {
      snprintf( buf, MAX_STRING_LENGTH,
             "<tr><td><font color=\"#008080\">%s</font></td><td><font color=\"blue\">%s</font></td><td><font color=\"green\">%s</font></td><td><font color=\"#00FF00\">%s</font></td></tr>",
             this_imcmud->localname, this_imcmud->versionid, this_imcmud->network, this_imcmud->servername );
   }
   else
   {
      imcstrlcpy( urldump, this_imcmud->www, MAX_INPUT_LENGTH );
      snprintf( buf, MAX_STRING_LENGTH,
             "<tr><td><font color=\"#008080\"><a href=\"%s\" class=\"dcyan\" target=\"_blank\">%s</a></font></td><td><font color=\"blue\">%s</font></td><td><font color=\"green\">%s</font></td><td><font color=\"#00FF00\">%s</font></td></tr>",
             urldump, this_imcmud->localname, this_imcmud->versionid, this_imcmud->network, this_imcmud->servername );
   }
   send_buf( wdesc->fd, buf );

   for( rin = first_rinfo; rin; rin = rin->next, count++ )
   {
      if( !str_cmp( rin->network, "unknown" ) )
         imcstrlcpy( netname, this_imcmud->network, MAX_INPUT_LENGTH );
      else
         imcstrlcpy( netname, rin->network, MAX_INPUT_LENGTH );

      /* If there is more then one path use the second path */
      if( rin->path && rin->path[0] != '\0' )
      {
         if( ( start = strchr( rin->path, '!' ) ) != NULL )
         {
            start++;
            onpath = start;
            end = 0;
            for( onpath = start; *onpath != '!' && *onpath != '\0'; onpath++ )
            {
               serverpath[end] = *onpath;
               end++;
            }
            serverpath[end] = '\0';
         }
         else
            imcstrlcpy( serverpath, rin->path, MAX_INPUT_LENGTH );
      }

      if( !rin->url || !str_cmp( rin->url, "??" ) || !str_cmp( rin->url, "Unknown" )
       || !strstr( rin->url, "http://" ) )
      {
         snprintf( buf, MAX_STRING_LENGTH,
                   "<tr><td><font color=\"%s\">%s</font></td><td><font color=\"blue\">%s</font></td><td><font color=\"green\">%s</font></td><td><font color=\"#00FF00\">%s</font></td></tr>",
                   rin->expired ? "red" : "#008080", rin->name, rin->version, netname, serverpath );
      }
      else
      {
         imcstrlcpy( urldump, rin->url, MAX_INPUT_LENGTH );
         snprintf( buf, MAX_STRING_LENGTH,
                   "<tr><td><font color=\"%s\"><a href=\"%s\" class=\"%s\" target=\"_blank\">%s</a></font></td><td><font color=\"blue\">%s</font></td><td><font color=\"green\">%s</font></td><td><font color=\"#00FF00\">%s</font></td></tr>",
                   rin->expired ? "red" : "#008080", urldump, rin->expired ? "red" : "dcyan",
                   rin->name, rin->version, netname, serverpath );
      }
      send_buf( wdesc->fd, buf );
   }
   imcstrlcpy( buf,
               "<tr><td colspan=\"5\"><font color=\"white\">Red mud names indicate connections that are down.</font></td></tr>",
               MAX_STRING_LENGTH );
   send_buf( wdesc->fd, buf );
   snprintf( buf, MAX_STRING_LENGTH, "<tr><td colspan=\"5\">%d connections on %s found.</td></tr></table>", count,
             this_imcmud->network );
   send_buf( wdesc->fd, buf );
   return;
}
#endif

IMC_CMD( imclist )
{
   REMOTEINFO *p;
   char serverpath[LGST], netname[SMST];
   char *start, *onpath;
   int count = 1, end;

   /*
    * Silly little thing, but since imcchanlist <channel> works... why not? -- Xorith
    */
   if( argument && argument[0] != '\0' )
   {
      imcinfo( ch, argument );
      return;
   }

   imcpager_printf( ch, "~WActive muds on %s:~!\r\n", this_imcmud->network );
   imcpager_printf( ch, "~c%-15.15s ~B%-40.40s~! ~g%-15.15s ~G%s", "Name", "IMC2 Version", "Network", "Server" );

   /*
    * Put local mud on the list, why was this not done? It's a mud isn't it?
    */
   imcpager_printf( ch, "\r\n\r\n~c%-15.15s ~B%-40.40s ~g%-15.15s ~G%s",
                    this_imcmud->localname, this_imcmud->versionid, this_imcmud->network, this_imcmud->servername );

   for( p = first_rinfo; p; p = p->next, count++ )
   {
      if( !strcasecmp( p->network, "unknown" ) )
         imcstrlcpy( netname, this_imcmud->network, SMST );
      else
         imcstrlcpy( netname, p->network, SMST );
      /*
       * If there is more then one path use the second path
       */
      if( p->path && p->path[0] != '\0' )
      {
         if( ( start = strchr( p->path, '!' ) ) != NULL )
         {
            start++;
            onpath = start;
            end = 0;
            for( onpath = start; *onpath != '!' && *onpath != '\0'; onpath++ )
            {
               serverpath[end] = *onpath;
               end++;
            }
            serverpath[end] = '\0';
         }
         else
            imcstrlcpy( serverpath, p->path, LGST );
      }
      imcpager_printf( ch, "\r\n~%c%-15.15s ~B%-40.40s ~g%-15.15s ~G%s",
                       p->expired ? 'R' : 'c', p->name, p->version, netname, serverpath );
   }
   imcpager_printf( ch, "\r\n~WRed mud names indicate connections that are down." );
   imcpager_printf( ch, "\r\n~W%d muds on %s found.\r\n", count, this_imcmud->network );
}

IMC_CMD( imcconnect )
{
   if( this_imcmud && this_imcmud->state > IMC_OFFLINE )
   {
      imc_to_char( "The IMC2 network connection appears to already be engaged!\r\n", ch );
      return;
   }
   imcconnect_attempts = 0;
   imcwait = 0;
   imc_startup( TRUE, -1, FALSE );
   return;
}

IMC_CMD( imcdisconnect )
{
   if( this_imcmud && this_imcmud->state == IMC_OFFLINE )
   {
      imc_to_char( "The IMC2 network connection does not appear to be engaged!\r\n", ch );
      return;
   }
   imc_shutdown( FALSE );
   return;
}

IMC_CMD( imcconfig )
{
   char arg1[SMST];

   argument = imcone_argument( argument, arg1 );

   if( arg1[0] == '\0' )
   {
      imc_to_char( "~wSyntax: &Gimc <field> [value]\r\n\r\n", ch );
      imc_to_char( "~wConfiguration info for your mud. Changes save when edited.\r\n", ch );
      imc_to_char( "~wYou may set the following:\r\n\r\n", ch );
      imc_to_char( "~wShow           : ~GDisplays your current configuration.\r\n", ch );
      imc_to_char( "~wLocalname      : ~GThe name IMC2 knows your mud by.\r\n", ch );
      imc_to_char( "~wAutoconnect    : ~GToggles automatic connection on reboots.\r\n", ch );
      imc_to_char( "~wMinPlayerLevel : ~GSets the minimum level IMC2 can see your players at.\r\n", ch );
      imc_to_char( "~wMinImmLevel    : ~GSets the level at which immortal commands become available.\r\n", ch );
      imc_to_char( "~wAdminlevel     : ~GSets the level at which administrative commands become available.\r\n", ch );
      imc_to_char( "~wImplevel       : ~GSets the level at which immplementor commands become available.\r\n", ch );
      imc_to_char( "~wInfoname       : ~GName of your mud, as seen from the imcquery info sheet.\r\n", ch );
      imc_to_char( "~wInfohost       : ~GTelnet address of your mud.\r\n", ch );
      imc_to_char( "~wInfoport       : ~GTelnet port of your mud.\r\n", ch );
      imc_to_char( "~wInfoemail      : ~GEmail address of the mud's IMC administrator.\r\n", ch );
      imc_to_char( "~wInfoWWW        : ~GThe Web address of your mud.\r\n", ch );
      imc_to_char( "~wInfoBase       : ~GThe codebase your mud uses.\r\n", ch );
      imc_to_char( "~wInfoDetails    : ~GSHORT Description of your mud.\r\n", ch );
      imc_to_char( "~wServerAddr     : ~GDNS or IP address of the server you mud connects to.\r\n", ch );
      imc_to_char( "~wServerPort     : ~GPort of the server your mud connects to.\r\n", ch );
      imc_to_char( "~wClientPwd      : ~GClient password for your mud.\r\n", ch );
      imc_to_char( "~wServerPwd      : ~GServer password for your mud.\r\n", ch );
      return;
   }

   if( !strcasecmp( arg1, "sha256" ) )
   {
      this_imcmud->sha256 = !this_imcmud->sha256;

      if( this_imcmud->sha256 )
         imc_to_char( "SHA-256 support enabled.\r\n", ch );
      else
         imc_to_char( "SHA-256 support disabled.\r\n", ch );
      imc_save_config(  );
      return;
   }

   if( !strcasecmp( arg1, "sha256pass" ) )
   {
      this_imcmud->sha256pass = !this_imcmud->sha256pass;

      if( this_imcmud->sha256pass )
         imc_to_char( "SHA-256 Authentication enabled.\r\n", ch );
      else
         imc_to_char( "SHA-256 Authentication disabled.\r\n", ch );
      imc_save_config(  );
      return;
   }

   if( !strcasecmp( arg1, "autoconnect" ) )
   {
      this_imcmud->autoconnect = !this_imcmud->autoconnect;

      if( this_imcmud->autoconnect )
         imc_to_char( "Autoconnect enabled.\r\n", ch );
      else
         imc_to_char( "Autoconnect disabled.\r\n", ch );
      imc_save_config(  );
      return;
   }

   if( !strcasecmp( arg1, "show" ) )
   {
      imc_printf( ch, "~wLocalname      : ~G%s\r\n", this_imcmud->localname );
      imc_printf( ch, "~wAutoconnect    : ~G%s\r\n", this_imcmud->autoconnect ? "Enabled" : "Disabled" );
      imc_printf( ch, "~wMinPlayerLevel : ~G%d\r\n", this_imcmud->minlevel );
      imc_printf( ch, "~wMinImmLevel    : ~G%d\r\n", this_imcmud->immlevel );
      imc_printf( ch, "~wAdminlevel     : ~G%d\r\n", this_imcmud->adminlevel );
      imc_printf( ch, "~wImplevel       : ~G%d\r\n", this_imcmud->implevel );
      imc_printf( ch, "~wInfoname       : ~G%s\r\n", this_imcmud->fullname );
      imc_printf( ch, "~wInfohost       : ~G%s\r\n", this_imcmud->ihost );
      imc_printf( ch, "~wInfoport       : ~G%d\r\n", this_imcmud->iport );
      imc_printf( ch, "~wInfoemail      : ~G%s\r\n", this_imcmud->email );
      imc_printf( ch, "~wInfoWWW        : ~G%s\r\n", this_imcmud->www );
      imc_printf( ch, "~wInfoBase       : ~G%s\r\n", this_imcmud->base );
      imc_printf( ch, "~wInfoDetails    : ~G%s\r\n\r\n", this_imcmud->details );
      imc_printf( ch, "~wServerAddr     : ~G%s\r\n", this_imcmud->rhost );
      imc_printf( ch, "~wServerPort     : ~G%d\r\n", this_imcmud->rport );
      imc_printf( ch, "~wClientPwd      : ~G%s\r\n", this_imcmud->clientpw );
      imc_printf( ch, "~wServerPwd      : ~G%s\r\n", this_imcmud->serverpw );
      if( this_imcmud->sha256 )
         imc_to_char( "~RThis mud has enabled SHA-256 authentication.\r\n", ch );
      else
         imc_to_char( "~RThis mud has disabled SHA-256 authentication.\r\n", ch );
      if( this_imcmud->sha256 && this_imcmud->sha256pass )
         imc_to_char( "~RThe mud is using SHA-256 encryption to authenticate.\r\n", ch );
      else
         imc_to_char( "~RThe mud is using plain text passwords to authenticate.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      imcconfig( ch, "" );
      return;
   }

   if( !strcasecmp( arg1, "minplayerlevel" ) )
   {
      int value = atoi( argument );

      imc_printf( ch, "Minimum level set to %d\r\n", value );
      this_imcmud->minlevel = value;
      imc_save_config(  );
      return;
   }

   if( !strcasecmp( arg1, "minimmlevel" ) )
   {
      int value = atoi( argument );

      imc_printf( ch, "Immortal level set to %d\r\n", value );
      this_imcmud->immlevel = value;
      imc_save_config(  );
      return;
   }

   if( !strcasecmp( arg1, "adminlevel" ) )
   {
      int value = atoi( argument );

      imc_printf( ch, "Admin level set to %d\r\n", value );
      this_imcmud->adminlevel = value;
      imc_save_config(  );
      return;
   }

   if( !strcasecmp( arg1, "implevel" ) && IMCPERM( ch ) == IMCPERM_IMP )
   {
      int value = atoi( argument );

      imc_printf( ch, "Implementor level set to %d\r\n", value );
      this_imcmud->implevel = value;
      imc_save_config(  );
      return;
   }

   if( !strcasecmp( arg1, "infoname" ) )
   {
      IMCSTRFREE( this_imcmud->fullname );
      this_imcmud->fullname = IMCSTRALLOC( argument );
      imc_save_config(  );
      imc_printf( ch, "Infoname change to %s\r\n", argument );
      return;
   }

   if( !strcasecmp( arg1, "infohost" ) )
   {
      IMCSTRFREE( this_imcmud->ihost );
      this_imcmud->ihost = IMCSTRALLOC( argument );
      imc_save_config(  );
      imc_printf( ch, "Infohost changed to %s\r\n", argument );
      return;
   }

   if( !strcasecmp( arg1, "infoport" ) )
   {
      this_imcmud->iport = atoi( argument );
      imc_save_config(  );
      imc_printf( ch, "Infoport changed to %d\r\n", this_imcmud->iport );
      return;
   }

   if( !strcasecmp( arg1, "infoemail" ) )
   {
      IMCSTRFREE( this_imcmud->email );
      this_imcmud->email = IMCSTRALLOC( argument );
      imc_save_config(  );
      imc_printf( ch, "Infoemail changed to %s\r\n", argument );
      return;
   }

   if( !strcasecmp( arg1, "infowww" ) )
   {
      IMCSTRFREE( this_imcmud->www );
      this_imcmud->www = IMCSTRALLOC( argument );
      imc_save_config(  );
      imc_printf( ch, "InfoWWW changed to %s\r\n", argument );
      imc_send_keepalive( NULL, "*@*" );
      return;
   }

   if( !strcasecmp( arg1, "infobase" ) )
   {
      char cbase[SMST];

      IMCSTRFREE( this_imcmud->base );
      this_imcmud->base = IMCSTRALLOC( argument );
      imc_save_config(  );
      imc_printf( ch, "Infobase changed to %s\r\n", argument );

      IMCSTRFREE( this_imcmud->versionid );
      snprintf( cbase, SMST, "%s%s", IMC_VERSION_STRING, this_imcmud->base );
      this_imcmud->versionid = IMCSTRALLOC( cbase );
      imc_send_keepalive( NULL, "*@*" );
      return;
   }

   if( !strcasecmp( arg1, "infodetails" ) )
   {
      IMCSTRFREE( this_imcmud->details );
      this_imcmud->details = IMCSTRALLOC( argument );
      imc_save_config(  );
      imc_to_char( "Infodetails updated.\r\n", ch );
      return;
   }

   if( this_imcmud->state != IMC_OFFLINE )
   {
      imc_printf( ch, "Cannot alter %s while the mud is connected to IMC.\r\n", arg1 );
      return;
   }

   if( !strcasecmp( arg1, "serveraddr" ) )
   {
      IMCSTRFREE( this_imcmud->rhost );
      this_imcmud->rhost = IMCSTRALLOC( argument );
      imc_printf( ch, "ServerAddr changed to %s\r\n", argument );
      imc_save_config(  );
      return;
   }

   if( !strcasecmp( arg1, "serverport" ) )
   {
      this_imcmud->rport = atoi( argument );
      imc_printf( ch, "ServerPort changed to %d\r\n", this_imcmud->rport );
      imc_save_config(  );
      return;
   }

   if( !strcasecmp( arg1, "clientpwd" ) )
   {
      IMCSTRFREE( this_imcmud->clientpw );
      this_imcmud->clientpw = IMCSTRALLOC( argument );
      imc_printf( ch, "Clientpwd changed to %s\r\n", argument );
      imc_save_config(  );
      return;
   }

   if( !strcasecmp( arg1, "serverpwd" ) )
   {
      IMCSTRFREE( this_imcmud->serverpw );
      this_imcmud->serverpw = IMCSTRALLOC( argument );
      imc_printf( ch, "Serverpwd changed to %s\r\n", argument );
      imc_save_config(  );
      return;
   }

   if( !strcasecmp( arg1, "localname" ) )
   {
      IMCSTRFREE( this_imcmud->localname );
      this_imcmud->localname = IMCSTRALLOC( argument );
      this_imcmud->sha256pass = FALSE;
      imc_save_config(  );
      imc_printf( ch, "Localname changed to %s\r\n", argument );
      return;
   }
   imcconfig( ch, "" );
   return;
}

/* Modified this command so it's a little more helpful -- Xorith */
IMC_CMD( imcignore )
{
   int count;
   IMC_IGNORE *ign;
   char arg[SMST];

   argument = imcone_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      imc_to_char( "You currently ignore the following:\r\n", ch );
      for( count = 0, ign = FIRST_IMCIGNORE( ch ); ign; ign = ign->next, count++ )
         imc_printf( ch, "%s\r\n", ign->name );

      if( !count )
         imc_to_char( " none\r\n", ch );
      else
         imc_printf( ch, "\r\n[total %d]\r\n", count );
      imc_to_char( "For help on imcignore, type: IMCIGNORE HELP\r\n", ch );
      return;
   }

   if( !strcasecmp( arg, "help" ) )
   {
      imc_to_char( "~wTo see your current ignores  : ~GIMCIGNORE\r\n", ch );
      imc_to_char( "~wTo add an ignore             : ~GIMCIGNORE ADD <argument>\r\n", ch );
      imc_to_char( "~wTo delete an ignore          : ~GIMCIGNORE DELETE <argument>\r\n", ch );
      imc_to_char( "~WSee your MUD's help for more information.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      imc_to_char( "Must specify both action and name.\r\n", ch );
      imc_to_char( "Please see IMCIGNORE HELP for details.\r\n", ch );
      return;
   }

   if( !strcasecmp( arg, "delete" ) )
   {
      for( ign = FIRST_IMCIGNORE( ch ); ign; ign = ign->next )
      {
         if( !strcasecmp( ign->name, argument ) )
         {
            IMCUNLINK( ign, FIRST_IMCIGNORE( ch ), LAST_IMCIGNORE( ch ), next, prev );
            IMCSTRFREE( ign->name );
            IMCDISPOSE( ign );
            imc_to_char( "Entry deleted.\r\n", ch );
            return;
         }
      }
      imc_to_char( "Entry not found.\r\nPlease check your ignores by typing IMCIGNORE with no arguments.\r\n", ch );
      return;
   }

   if( !strcasecmp( arg, "add" ) )
   {
      IMCCREATE( ign, IMC_IGNORE, 1 );
      ign->name = IMCSTRALLOC( argument );
      IMCLINK( ign, FIRST_IMCIGNORE( ch ), LAST_IMCIGNORE( ch ), next, prev );
      imc_printf( ch, "%s will now be ignored.\r\n", argument );
      return;
   }
   imcignore( ch, "help" );
   return;
}

/* Made this command a little more helpful --Xorith */
IMC_CMD( imcban )
{
   int count;
   IMC_BAN *ban;
   char arg[SMST];

   argument = imcone_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      imc_to_char( "The mud currently bans the following:\r\n", ch );
      for( count = 0, ban = first_imc_ban; ban; ban = ban->next, count++ )
         imc_printf( ch, "%s\r\n", ban->name );

      if( !count )
         imc_to_char( " none\r\n", ch );
      else
         imc_printf( ch, "\r\n[total %d]\r\n", count );
      imc_to_char( "Type: IMCBAN HELP for more information.\r\n", ch );
      return;
   }

   if( !strcasecmp( arg, "help" ) )
   {
      imc_to_char( "~wTo see the current bans             : ~GIMCBAN\r\n", ch );
      imc_to_char( "~wTo add a MUD to the ban list        : ~GIMCBAN ADD <argument>\r\n", ch );
      imc_to_char( "~wTo delete a MUD from the ban list   : ~GIMCBAN DELETE <argument>\r\n", ch );
      imc_to_char( "~WSee your MUD's help for more information.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      imc_to_char( "Must specify both action and name.\r\nPlease type IMCBAN HELP for more information\r\n", ch );
      return;
   }

   if( !strcasecmp( arg, "delete" ) )
   {
      if( imc_delban( argument ) )
      {
         imc_savebans(  );
         imc_to_char( "Entry deleted.\r\n", ch );
         return;
      }
      imc_to_char( "Entry not found.\r\nPlease type IMCBAN without arguments to see the current ban list.\r\n", ch );
   }

   if( !strcasecmp( arg, "add" ) )
   {
      imc_addban( argument );
      imc_savebans(  );
      imc_printf( ch, "Mud %s will now be banned.\r\n", argument );
      return;
   }
   imcban( ch, "" );
   return;
}

IMC_CMD( imc_deny_channel )
{
   char vic_name[SMST];
   CHAR_DATA *victim;
   IMC_CHANNEL *channel;

   argument = imcone_argument( argument, vic_name );

   if( vic_name[0] == '\0' || argument[0] == '\0' )
   {
      imc_to_char( "Usage: imcdeny <person> <local channel name>\r\n", ch );
      imc_to_char( "Usage: imcdeny <person> [tell/beep/finger]\r\n", ch );
      return;
   }

   if( !( victim = imc_find_user( vic_name ) ) )
   {
      imc_to_char( "No such person is currently online.\r\n", ch );
      return;
   }

   if( IMCPERM( ch ) <= IMCPERM( victim ) )
   {
      imc_to_char( "You cannot alter their settings.\r\n", ch );
      return;
   }

   if( !strcasecmp( argument, "tell" ) )
   {
      if( !IMCIS_SET( IMCFLAG( victim ), IMC_DENYTELL ) )
      {
         IMCSET_BIT( IMCFLAG( victim ), IMC_DENYTELL );
         imc_printf( ch, "%s can no longer use imctells.\r\n", CH_IMCNAME( victim ) );
         return;
      }
      IMCREMOVE_BIT( IMCFLAG( victim ), IMC_DENYTELL );
      imc_printf( ch, "%s can use imctells again.\r\n", CH_IMCNAME( victim ) );
      return;
   }

   if( !strcasecmp( argument, "beep" ) )
   {
      if( !IMCIS_SET( IMCFLAG( victim ), IMC_DENYBEEP ) )
      {
         IMCSET_BIT( IMCFLAG( victim ), IMC_DENYBEEP );
         imc_printf( ch, "%s can no longer use imcbeeps.\r\n", CH_IMCNAME( victim ) );
         return;
      }
      IMCREMOVE_BIT( IMCFLAG( victim ), IMC_DENYBEEP );
      imc_printf( ch, "%s can use imcbeeps again.\r\n", CH_IMCNAME( victim ) );
      return;
   }

   if( !strcasecmp( argument, "finger" ) )
   {
      if( !IMCIS_SET( IMCFLAG( victim ), IMC_DENYFINGER ) )
      {
         IMCSET_BIT( IMCFLAG( victim ), IMC_DENYFINGER );
         imc_printf( ch, "%s can no longer use imcfingers.\r\n", CH_IMCNAME( victim ) );
         return;
      }
      IMCREMOVE_BIT( IMCFLAG( victim ), IMC_DENYFINGER );
      imc_printf( ch, "%s can use imcfingers again.\r\n", CH_IMCNAME( victim ) );
      return;
   }

   /*
    * Assumed to be denying a channel by this stage.
    */
   if( !( channel = imc_findchannel( argument ) ) )
   {
      imc_to_char( "Unknown or unconfigured local channel. Check your channel name.\r\n", ch );
      return;
   }

   if( imc_hasname( IMC_DENY( victim ), channel->local_name ) )
   {
      imc_printf( ch, "%s can now listen to %s\r\n", CH_IMCNAME( victim ), channel->local_name );
      imc_removename( &IMC_DENY( victim ), channel->local_name );
   }
   else
   {
      imc_printf( ch, "%s can no longer listen to %s\r\n", CH_IMCNAME( victim ), channel->local_name );
      imc_addname( &IMC_DENY( victim ), channel->local_name );
   }
   return;
}

IMC_CMD( imcpermstats )
{
   CHAR_DATA *victim;

   if( !argument || argument[0] == '\0' )
   {
      imc_to_char( "Usage: imcperms <user>\r\n", ch );
      return;
   }

   if( !( victim = imc_find_user( argument ) ) )
   {
      imc_to_char( "No such person is currently online.\r\n", ch );
      return;
   }

   if( IMCPERM( victim ) < 0 || IMCPERM( victim ) > IMCPERM_IMP )
   {
      imc_printf( ch, "%s has an invalid permission setting!\r\n", CH_IMCNAME( victim ) );
      return;
   }

   imc_printf( ch, "~GPermissions for %s: %s\r\n", CH_IMCNAME( victim ), imcperm_names[IMCPERM( victim )] );
   imc_printf( ch, "~gThese permissions were obtained %s.\r\n",
               IMCIS_SET( IMCFLAG( victim ), IMC_PERMOVERRIDE ) ? "manually via imcpermset" : "automatically by level" );
   return;
}

IMC_CMD( imcpermset )
{
   CHAR_DATA *victim;
   char arg[SMST];
   int permvalue;

   argument = imcone_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      imc_to_char( "Usage: imcpermset <user> <permission>\r\n", ch );
      imc_to_char( "Permission can be one of: None, Mort, Imm, Admin, Imp\r\n", ch );
      return;
   }

   if( !( victim = imc_find_user( arg ) ) )
   {
      imc_to_char( "No such person is currently online.\r\n", ch );
      return;
   }

   if( !strcasecmp( argument, "override" ) )
      permvalue = -1;
   else
   {
      permvalue = get_imcpermvalue( argument );

      if( !imccheck_permissions( ch, permvalue, IMCPERM( victim ), TRUE ) )
         return;
   }

   /*
    * Just something to avoid looping through the channel clean-up --Xorith
    */
   if( IMCPERM( victim ) == permvalue )
   {
      imc_printf( ch, "%s already has a permission level of %s.\r\n", CH_IMCNAME( victim ), imcperm_names[permvalue] );
      return;
   }

   if( permvalue == -1 )
   {
      IMCREMOVE_BIT( IMCFLAG( victim ), IMC_PERMOVERRIDE );
      imc_printf( ch, "~YPermission flag override has been removed from %s\r\n", CH_IMCNAME( victim ) );
      return;
   }

   IMCPERM( victim ) = permvalue;
   IMCSET_BIT( IMCFLAG( victim ), IMC_PERMOVERRIDE );

   imc_printf( ch, "~YPermission level for %s has been changed to %s\r\n", CH_IMCNAME( victim ), imcperm_names[permvalue] );
   /*
    * Channel Clean-Up added by Xorith 9-24-03
    */
   /*
    * Note: Let's not clean up IMC_DENY for a player. Never know...
    */
   if( IMC_LISTEN( victim ) != NULL && this_imcmud->state == IMC_ONLINE )
   {
      IMC_CHANNEL *channel = NULL;
      char *channels = IMC_LISTEN( victim );

      while( 1 )
      {
         if( channels[0] == '\0' )
            break;
         channels = imcone_argument( channels, arg );

         if( !( channel = imc_findchannel( arg ) ) )
            imc_removename( &IMC_LISTEN( victim ), arg );
         if( channel && IMCPERM( victim ) < channel->level )
         {
            imc_removename( &IMC_LISTEN( victim ), arg );
            imc_printf( ch, "~WRemoving '%s' level channel: '%s', exceeding new permission of '%s'\r\n",
                        imcperm_names[channel->level], channel->local_name, imcperm_names[IMCPERM( victim )] );
         }
      }
   }
   return;
}

IMC_CMD( imcinvis )
{
   if( IMCIS_SET( IMCFLAG( ch ), IMC_INVIS ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_INVIS );
      imc_to_char( "You are now imcvisible.\r\n", ch );
   }
   else
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_INVIS );
      imc_to_char( "You are now imcinvisible.\r\n", ch );
   }
   return;
}

IMC_CMD( imcchanwho )
{
   IMC_CHANNEL *c;
   IMC_PACKET *p;
   char chan[SMST], mud[SMST];

   if( !argument || argument[0] == '\0' )
   {
      imc_to_char( "Usage: imcchanwho <channel> [<mud> <mud> <mud> <...>|<all>]\r\n", ch );
      return;
   }

   argument = imcone_argument( argument, chan );

   if( !( c = imc_findchannel( chan ) ) )
   {
      imc_to_char( "No such channel.\r\n", ch );
      return;
   }

   if( IMCPERM( ch ) < c->level )
   {
      imc_to_char( "No such channel.\r\n", ch );
      return;
   }

   if( !c->refreshed )
   {
      imc_printf( ch, "%s has not been refreshed yet.\r\n", c->name );
      return;
   }

   if( strcasecmp( argument, "all" ) )
   {
      while( argument[0] != '\0' )
      {
         argument = imcone_argument( argument, mud );

         if( !check_mud( ch, mud ) )
            continue;

         p = imc_newpacket( CH_IMCNAME( ch ), "ice-chan-who", mud );
         imc_addtopacket( p, "level=%d", IMCPERM( ch ) );
         imc_addtopacket( p, "channel=%s", c->name );
         imc_addtopacket( p, "lname=%s", c->local_name ? c->local_name : c->name );
         imc_write_packet( p );
      }
      return;
   }

   p = imc_newpacket( CH_IMCNAME( ch ), "ice-chan-who", "*" );
   imc_addtopacket( p, "level=%d", IMCPERM( ch ) );
   imc_addtopacket( p, "channel=%s", c->name );
   imc_addtopacket( p, "lname=%s", c->local_name ? c->local_name : c->name );
   imc_write_packet( p );

   imc_printf( ch, "~G%s", get_local_chanwho( c ) );
   return;
}

IMC_CMD( imcremoteadmin )
{
   REMOTEINFO *r;
   char server[SMST], cmd[SMST], to[SMST];
   char pwd[LGST];
   IMC_PACKET *p;

   argument = imcone_argument( argument, server );
   argument = imcone_argument( argument, pwd );
   argument = imcone_argument( argument, cmd );

   if( server[0] == '\0' || cmd[0] == '\0' )
   {
      imc_to_char( "Syntax: imcadmin <server> <password> <command> [<data..>]\r\n", ch );
      imc_to_char( "You must be an approved server administrator to use remote commands.\r\n", ch );
      return;
   }

   if( !( r = imc_find_reminfo( server ) ) )
   {
      imc_printf( ch, "~W%s ~cis not a valid mud name.\r\n", server );
      return;
   }

   if( r->expired )
   {
      imc_printf( ch, "~W%s ~cis not connected right now.\r\n", r->name );
      return;
   }

   snprintf( to, SMST, "IMC@%s", r->name );
   p = imc_newpacket( CH_IMCNAME( ch ), "remote-admin", to );
   imc_addtopacket( p, "command=%s", cmd );
   if( argument && argument[0] != '\0' )
      imc_addtopacket( p, "data=%s", argument );
   if( this_imcmud->sha256pass )
   {
      char cryptpw[LGST];
      char *hash;

      snprintf( cryptpw, LGST, "%ld%s", imc_sequencenumber + 1, pwd );
      hash = sha256_crypt( cryptpw );
      imc_addtopacket( p, "hash=%s", hash );
   }
   imc_write_packet( p );

   imc_to_char( "Remote command sent.\r\n", ch );
   return;
}

IMC_CMD( imchelp )
{
   char buf[LGST];
   IMC_HELP_DATA *help;
   int col, perm;

   if( !argument || argument[0] == '\0' )
   {
      imcstrlcpy( buf, "~gHelp is available for the following commands:\r\n", LGST );
      imcstrlcat( buf, "~G---------------------------------------------\r\n", LGST );
      for( perm = IMCPERM_MORT; perm <= IMCPERM( ch ); perm++ )
      {
         col = 0;
         snprintf( buf + strlen( buf ), LGST - strlen( buf ), "\r\n~g%s helps:~G\r\n", imcperm_names[perm] );
         for( help = first_imc_help; help; help = help->next )
         {
            if( help->level != perm )
               continue;

            snprintf( buf + strlen( buf ), LGST - strlen( buf ), "%-15s", help->name );
            if( ++col % 6 == 0 )
               imcstrlcat( buf, "\r\n", LGST );
         }
         if( col % 6 != 0 )
            imcstrlcat( buf, "\r\n", LGST );
      }
      imc_to_pager( buf, ch );
      return;
   }

   for( help = first_imc_help; help; help = help->next )
   {
      if( !strcasecmp( help->name, argument ) )
      {
         if( !help->text || help->text[0] == '\0' )
            imc_printf( ch, "~gNo inforation available for topic ~W%s~g.\r\n", help->name );
         else
            imc_printf( ch, "~g%s\r\n", help->text );
         return;
      }
   }
   imc_printf( ch, "~gNo help exists for topic ~W%s~g.\r\n", argument );
   return;
}

IMC_CMD( imccolor )
{
   if( IMCIS_SET( IMCFLAG( ch ), IMC_COLORFLAG ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_COLORFLAG );
      imc_to_char( "IMC2 color is now off.\r\n", ch );
   }
   else
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_COLORFLAG );
      imc_to_char( "~RIMC2 c~Yo~Gl~Bo~Pr ~Ris now on. Enjoy :)\r\n", ch );
   }
   return;
}

IMC_CMD( imcafk )
{
   if( IMCIS_SET( IMCFLAG( ch ), IMC_AFK ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_AFK );
      imc_to_char( "You are no longer AFK to IMC2.\r\n", ch );
   }
   else
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_AFK );
      imc_to_char( "You are now AFK to IMC2.\r\n", ch );
   }
   return;
}

IMC_CMD( imcdebug )
{
   imcpacketdebug = !imcpacketdebug;

   if( imcpacketdebug )
      imc_to_char( "Packet debug enabled.\r\n", ch );
   else
      imc_to_char( "Packet debug disabled.\r\n", ch );
   return;
}

/* This is very possibly going to be spammy as hell */
IMC_CMD( imc_show_ucache_contents )
{
   IMCUCACHE_DATA *user;
   int users = 0;

   imc_to_pager( "Cached user information\r\n", ch );
   imc_to_pager( "User                          | Gender ( 0 = Male, 1 = Female, 2 = Other )\r\n", ch );
   imc_to_pager( "--------------------------------------------------------------------------\r\n", ch );
   for( user = first_imcucache; user; user = user->next )
   {
      imcpager_printf( ch, "%-30s %d\r\n", user->name, user->gender );
      users++;
   }
   imcpager_printf( ch, "%d users being cached.\r\n", users );
   return;
}

IMC_CMD( imccedit )
{
   IMC_CMD_DATA *cmd, *tmp;
   IMC_ALIAS *alias, *alias_next;
   char name[SMST], option[SMST];
   bool found = FALSE, aliasfound = FALSE;

   argument = imcone_argument( argument, name );
   argument = imcone_argument( argument, option );

   if( name[0] == '\0' || option[0] == '\0' )
   {
      imc_to_char( "Usage: imccedit <command> <create|delete|alias|rename|code|permission|connected> <field>.\r\n", ch );
      return;
   }

   for( cmd = first_imc_command; cmd; cmd = cmd->next )
   {
      if( !strcasecmp( cmd->name, name ) )
      {
         found = TRUE;
         break;
      }
      for( alias = cmd->first_alias; alias; alias = alias->next )
      {
         if( !strcasecmp( alias->name, name ) )
            aliasfound = TRUE;
      }
   }

   if( !strcasecmp( option, "create" ) )
   {
      if( found )
      {
         imc_printf( ch, "~gA command named ~W%s ~galready exists.\r\n", name );
         return;
      }

      if( aliasfound )
      {
         imc_printf( ch, "~g%s already exists as an alias for another command.\r\n", name );
         return;
      }

      IMCCREATE( cmd, IMC_CMD_DATA, 1 );
      cmd->name = IMCSTRALLOC( name );
      cmd->level = IMCPERM( ch );
      cmd->connected = FALSE;
      imc_printf( ch, "~gCommand ~W%s ~gcreated.\r\n", cmd->name );
      if( argument && argument[0] != '\0' )
      {
         cmd->function = imc_function( argument );
         if( cmd->function == NULL )
            imc_printf( ch, "~gFunction ~W%s ~gdoes not exist - set to NULL.\r\n", argument );
      }
      else
      {
         imc_to_char( "~gFunction set to NULL.\r\n", ch );
         cmd->function = NULL;
      }
      IMCLINK( cmd, first_imc_command, last_imc_command, next, prev );
      imc_savecommands(  );
      return;
   }

   if( !found )
   {
      imc_printf( ch, "~gNo command named ~W%s ~gexists.\r\n", name );
      return;
   }

   if( !imccheck_permissions( ch, cmd->level, cmd->level, FALSE ) )
      return;

   if( !strcasecmp( option, "delete" ) )
   {
      imc_printf( ch, "~gCommand ~W%s ~ghas been deleted.\r\n", cmd->name );
      for( alias = cmd->first_alias; alias; alias = alias_next )
      {
         alias_next = alias->next;

         IMCUNLINK( alias, cmd->first_alias, cmd->last_alias, next, prev );
         IMCSTRFREE( alias->name );
         IMCDISPOSE( alias );
      }
      IMCUNLINK( cmd, first_imc_command, last_imc_command, next, prev );
      IMCSTRFREE( cmd->name );
      IMCDISPOSE( cmd );
      imc_savecommands(  );
      return;
   }

   /*
    * MY GOD! What an inefficient mess you've made Samson!
    */
   if( !strcasecmp( option, "alias" ) )
   {
      for( alias = cmd->first_alias; alias; alias = alias_next )
      {
         alias_next = alias->next;

         if( !strcasecmp( alias->name, argument ) )
         {
            imc_printf( ch, "~W%s ~ghas been removed as an alias for ~W%s\r\n", argument, cmd->name );
            IMCUNLINK( alias, cmd->first_alias, cmd->last_alias, next, prev );
            IMCSTRFREE( alias->name );
            IMCDISPOSE( alias );
            imc_savecommands(  );
            return;
         }
      }

      for( tmp = first_imc_command; tmp; tmp = tmp->next )
      {
         if( !strcasecmp( tmp->name, argument ) )
         {
            imc_printf( ch, "~W%s &gis already a command name.\r\n", argument );
            return;
         }
         for( alias = tmp->first_alias; alias; alias = alias->next )
         {
            if( !strcasecmp( argument, alias->name ) )
            {
               imc_printf( ch, "~W%s ~gis already an alias for ~W%s\r\n", argument, tmp->name );
               return;
            }
         }
      }

      IMCCREATE( alias, IMC_ALIAS, 1 );
      alias->name = IMCSTRALLOC( argument );
      IMCLINK( alias, cmd->first_alias, cmd->last_alias, next, prev );
      imc_printf( ch, "~W%s ~ghas been added as an alias for ~W%s\r\n", alias->name, cmd->name );
      imc_savecommands(  );
      return;
   }

   if( !strcasecmp( option, "connected" ) )
   {
      cmd->connected = !cmd->connected;

      if( cmd->connected )
         imc_printf( ch, "~gCommand ~W%s ~gwill now require a connection to IMC2 to use.\r\n", cmd->name );
      else
         imc_printf( ch, "~gCommand ~W%s ~gwill no longer require a connection to IMC2 to use.\r\n", cmd->name );
      imc_savecommands(  );
      return;
   }

   if( !strcasecmp( option, "show" ) )
   {
      char buf[LGST];

      imc_printf( ch, "~gCommand       : ~W%s\r\n", cmd->name );
      imc_printf( ch, "~gPermission    : ~W%s\r\n", imcperm_names[cmd->level] );
      imc_printf( ch, "~gFunction      : ~W%s\r\n", imc_funcname( cmd->function ) );
      imc_printf( ch, "~gConnection Req: ~W%s\r\n", cmd->connected ? "Yes" : "No" );
      if( cmd->first_alias )
      {
         int col = 0;
         imcstrlcpy( buf, "~gAliases       : ~W", LGST );
         for( alias = cmd->first_alias; alias; alias = alias->next )
         {
            snprintf( buf + strlen( buf ), LGST - strlen( buf ), "%s ", alias->name );
            if( ++col % 10 == 0 )
               imcstrlcat( buf, "\r\n", LGST );
         }
         if( col % 10 != 0 )
            imcstrlcat( buf, "\r\n", LGST );
         imc_to_char( buf, ch );
      }
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      imc_to_char( "Required argument missing.\r\n", ch );
      imccedit( ch, "" );
      return;
   }

   if( !strcasecmp( option, "rename" ) )
   {
      imc_printf( ch, "~gCommand ~W%s ~ghas been renamed to ~W%s.\r\n", cmd->name, argument );
      IMCSTRFREE( cmd->name );
      cmd->name = IMCSTRALLOC( argument );
      imc_savecommands(  );
      return;
   }

   if( !strcasecmp( option, "code" ) )
   {
      cmd->function = imc_function( argument );
      if( cmd->function == NULL )
         imc_printf( ch, "~gFunction ~W%s ~gdoes not exist - set to NULL.\r\n", argument );
      else
         imc_printf( ch, "~gFunction set to ~W%s.\r\n", argument );
      imc_savecommands(  );
      return;
   }

   if( !strcasecmp( option, "perm" ) || !strcasecmp( option, "permission" ) )
   {
      int permvalue = get_imcpermvalue( argument );

      if( !imccheck_permissions( ch, permvalue, cmd->level, FALSE ) )
         return;

      cmd->level = permvalue;
      imc_printf( ch, "~gCommand ~W%s ~gpermission level has been changed to ~W%s.\r\n", cmd->name,
                  imcperm_names[permvalue] );
      imc_savecommands(  );
      return;
   }
   imccedit( ch, "" );
   return;
}

IMC_CMD( imchedit )
{
   IMC_HELP_DATA *help;
   char name[SMST], cmd[SMST];
   bool found = FALSE;

   argument = imcone_argument( argument, name );
   argument = imcone_argument( argument, cmd );

   if( name[0] == '\0' || cmd[0] == '\0' || argument[0] == '\0' )
   {
      imc_to_char( "Usage: imchedit <topic> [name|perm] <field>\r\n", ch );
      imc_to_char( "Where <field> can be either name, or permission level.\r\n", ch );
      return;
   }

   for( help = first_imc_help; help; help = help->next )
   {
      if( !strcasecmp( help->name, name ) )
      {
         found = TRUE;
         break;
      }
   }

   if( !found )
   {
      imc_printf( ch, "~gNo help exists for topic ~W%s~g. You will need to add it to the helpfile manually.\r\n", name );
      return;
   }

   if( !strcasecmp( cmd, "name" ) )
   {
      imc_printf( ch, "~W%s ~ghas been renamed to ~W%s.\r\n", help->name, argument );
      IMCSTRFREE( help->name );
      help->name = IMCSTRALLOC( argument );
      imc_savehelps(  );
      return;
   }

   if( !strcasecmp( cmd, "perm" ) )
   {
      int permvalue = get_imcpermvalue( argument );

      if( !imccheck_permissions( ch, permvalue, help->level, FALSE ) )
         return;

      imc_printf( ch, "~gPermission level for ~W%s ~ghas been changed to ~W%s.\r\n", help->name, imcperm_names[permvalue] );
      help->level = permvalue;
      imc_savehelps(  );
      return;
   }
   imchedit( ch, "" );
   return;
}

IMC_CMD( imcnotify )
{
   if( IMCIS_SET( IMCFLAG( ch ), IMC_NOTIFY ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_NOTIFY );
      imc_to_char( "You no longer see channel notifications.\r\n", ch );
   }
   else
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_NOTIFY );
      imc_to_char( "You now see channel notifications.\r\n", ch );
   }
   return;
}

IMC_CMD( imcrefresh )
{
   REMOTEINFO *r, *rnext;

   for( r = first_rinfo; r; r = rnext )
   {
      rnext = r->next;
      imc_delete_reminfo( r );
   }
   imc_request_keepalive(  );
   imc_to_char( "Mud list is being refreshed.\r\n", ch );
   return;
}

IMC_CMD( imctemplates )
{
   imc_to_char( "Refreshing all templates.\r\n", ch );
   imc_load_templates();
}

IMC_CMD( imclast )
{
   IMC_PACKET *p;

   p = imc_newpacket( CH_IMCNAME( ch ), "imc-laston", this_imcmud->servername );
   if( argument && argument[0] != '\0' )
      imc_addtopacket( p, "username=%s", argument );
   imc_write_packet( p );

   return;
}

IMC_CMD( imc_other )
{
   char buf[LGST];
   IMC_CMD_DATA *cmd;
   int col, perm;

   imcstrlcpy( buf, "~gThe following commands are available:\r\n", LGST );
   imcstrlcat( buf, "~G-------------------------------------\r\n\r\n", LGST );
   for( perm = IMCPERM_MORT; perm <= IMCPERM( ch ); perm++ )
   {
      col = 0;
      snprintf( buf + strlen( buf ), LGST - strlen( buf ), "\r\n~g%s commands:~G\r\n", imcperm_names[perm] );
      for( cmd = first_imc_command; cmd; cmd = cmd->next )
      {
         if( cmd->level != perm )
            continue;

         snprintf( buf + strlen( buf ), LGST - strlen( buf ), "%-15s", cmd->name );
         if( ++col % 6 == 0 )
            imcstrlcat( buf, "\r\n", LGST );
      }
      if( col % 6 != 0 )
         imcstrlcat( buf, "\r\n", LGST );
   }
   imc_to_pager( buf, ch );
   imc_to_pager( "\r\n~gFor information about a specific command, see ~Wimchelp <command>~g.\r\n", ch );
   return;
}

#if defined(IMCSTANDALONE)
/* Standalone compile won't have social structure at all */
char *imc_send_social( CHAR_DATA *ch, char *argument, int telloption )
{
   return "";
}
#else
char *imc_find_social( CHAR_DATA * ch, char *sname, char *person, char *mud, int victim )
{
   static char socname[LGST];
#if defined(SMAUGSOCIAL)
   SOCIAL_DATA *social;
   char *c;
#else
   int cmd;
   bool found;
#endif

   socname[0] = '\0';

#if defined(SMAUGSOCIAL)
   for( c = sname; *c; *c = tolower( *c ), c++ );

   if( ( social = find_social( sname ) ) == NULL )
   {
      imc_printf( ch, "~YSocial ~W%s~Y does not exist on this mud.\r\n", sname );
      return socname;
   }

   if( person && person[0] != '\0' && mud && mud[0] != '\0' )
   {
      if( person && person[0] != '\0' && !strcasecmp( person, CH_IMCNAME( ch ) )
          && mud && mud[0] != '\0' && !strcasecmp( mud, this_imcmud->localname ) )
      {
         if( !social->others_auto )
         {
            imc_printf( ch, "~YSocial ~W%s~Y: Missing others_auto.\r\n", sname );
            return socname;
         }
         imcstrlcpy( socname, social->others_auto, LGST );
      }
      else
      {
         if( victim == 0 )
         {
            if( !social->others_found )
            {
               imc_printf( ch, "~YSocial ~W%s~Y: Missing others_found.\r\n", sname );
               return socname;
            }
            imcstrlcpy( socname, social->others_found, LGST );
         }
         else if( victim == 1 )
         {
            if( !social->vict_found )
            {
               imc_printf( ch, "~YSocial ~W%s~Y: Missing vict_found.\r\n", sname );
               return socname;
            }
            imcstrlcpy( socname, social->vict_found, LGST );
         }
         else
         {
            if( !social->char_found )
            {
               imc_printf( ch, "~YSocial ~W%s~Y: Missing char_found.\r\n", sname );
               return socname;
            }
            imcstrlcpy( socname, social->char_found, LGST );
         }
      }
   }
   else
   {
      if( victim == 0 || victim == 1 )
      {
         if( !social->others_no_arg )
         {
            imc_printf( ch, "~YSocial ~W%s~Y: Missing others_no_arg.\r\n", sname );
            return socname;
         }
         imcstrlcpy( socname, social->others_no_arg, LGST );
      }
      else
      {
         if( !social->char_no_arg )
         {
            imc_printf( ch, "~YSocial ~W%s~Y: Missing char_no_arg.\r\n", sname );
            return socname;
         }
         imcstrlcpy( socname, social->char_no_arg, LGST );
      }
   }
#else
   found = FALSE;
   for( cmd = 0; social_table[cmd].name[0] != '\0'; cmd++ )
   {
      if( sname[0] == social_table[cmd].name[0] && !imcstr_prefix( sname, social_table[cmd].name ) )
      {
         found = TRUE;
         break;
      }
   }

   if( !found )
   {
      imc_printf( ch, "~YSocial ~W%s~Y does not exist on this mud.\r\n", sname );
      return socname;
   }

   if( person && person[0] != '\0' && mud && mud[0] != '\0' )
   {
      if( person && person[0] != '\0' && !strcasecmp( person, CH_IMCNAME( ch ) )
          && mud && mud[0] != '\0' && !strcasecmp( mud, this_imcmud->localname ) )
      {
         if( !social_table[cmd].others_auto )
         {
            imc_printf( ch, "~YSocial ~W%s~Y: Missing others_auto.\r\n", sname );
            return socname;
         }
         imcstrlcpy( socname, social_table[cmd].others_auto, LGST );
      }
      else
      {
         if( victim == 0 )
         {
            if( !social_table[cmd].others_found )
            {
               imc_printf( ch, "~YSocial ~W%s~Y: Missing others_found.\r\n", sname );
               return socname;
            }
            imcstrlcpy( socname, social_table[cmd].others_found, LGST );
         }
         else if( victim == 1 )
         {
            if( !social_table[cmd].vict_found )
            {
               imc_printf( ch, "~YSocial ~W%s~Y: Missing vict_found.\r\n", sname );
               return socname;
            }
            imcstrlcpy( socname, social_table[cmd].vict_found, LGST );
         }
         else
         {
            if( !social_table[cmd].char_found )
            {
               imc_printf( ch, "~YSocial ~W%s~Y: Missing char_found.\r\n", sname );
               return socname;
            }
            imcstrlcpy( socname, social_table[cmd].char_found, LGST );
         }
      }
   }
   else
   {
      if( victim == 0 || victim == 1 )
      {
         if( !social_table[cmd].others_no_arg )
         {
            imc_printf( ch, "~YSocial ~W%s~Y: Missing others_no_arg.\r\n", sname );
            return socname;
         }
         imcstrlcpy( socname, social_table[cmd].others_no_arg, LGST );
      }
      else
      {
         if( !social_table[cmd].char_no_arg )
         {
            imc_printf( ch, "~YSocial ~W%s~Y: Missing char_no_arg.\r\n", sname );
            return socname;
         }
         imcstrlcpy( socname, social_table[cmd].char_no_arg, LGST );
      }
   }
#endif
   return socname;
}

/* Revised 10/10/03 by Xorith: Recognize the need to capitalize for a new sentence. */
char *imc_act_string( const char *format, CHAR_DATA * ch, CHAR_DATA * vic )
{
   static char *const he_she[] = { "it", "he", "she" };
   static char *const him_her[] = { "it", "him", "her" };
   static char *const his_her[] = { "its", "his", "her" };
   static char buf[LGST];
   char tmp_str[LGST];
   const char *i = "";
   char *point;
   bool should_upper = FALSE;

   if( !format || format[0] == '\0' || !ch )
      return NULL;

   point = buf;

   while( *format != '\0' )
   {
      if( *format == '.' || *format == '?' || *format == '!' )
         should_upper = TRUE;
      else if( should_upper == TRUE && !isspace( *format ) && *format != '$' )
         should_upper = FALSE;

      if( *format != '$' )
      {
         *point++ = *format++;
         continue;
      }
      ++format;

      if( ( !vic ) && ( *format == 'N' || *format == 'E' || *format == 'M' || *format == 'S' || *format == 'K' ) )
         i = " !!!!! ";
      else
      {
         switch ( *format )
         {
            default:
               i = " !!!!! ";
               break;
            case 'n':
               i = imc_makename( CH_IMCNAME( ch ), this_imcmud->localname );
               break;
            case 'N':
               i = CH_IMCNAME( vic );
               break;

            case 'e':
               i = should_upper ?
                  imccapitalize( he_she[URANGE( 0, CH_IMCSEX( ch ), 2 )] ) : he_she[URANGE( 0, CH_IMCSEX( ch ), 2 )];
               break;

            case 'E':
               i = should_upper ?
                  imccapitalize( he_she[URANGE( 0, CH_IMCSEX( vic ), 2 )] ) : he_she[URANGE( 0, CH_IMCSEX( vic ), 2 )];
               break;

            case 'm':
               i = should_upper ?
                  imccapitalize( him_her[URANGE( 0, CH_IMCSEX( ch ), 2 )] ) : him_her[URANGE( 0, CH_IMCSEX( ch ), 2 )];
               break;

            case 'M':
               i = should_upper ?
                  imccapitalize( him_her[URANGE( 0, CH_IMCSEX( vic ), 2 )] ) : him_her[URANGE( 0, CH_IMCSEX( vic ), 2 )];
               break;

            case 's':
               i = should_upper ?
                  imccapitalize( his_her[URANGE( 0, CH_IMCSEX( ch ), 2 )] ) : his_her[URANGE( 0, CH_IMCSEX( ch ), 2 )];
               break;

            case 'S':
               i = should_upper ?
                  imccapitalize( his_her[URANGE( 0, CH_IMCSEX( vic ), 2 )] ) : his_her[URANGE( 0, CH_IMCSEX( vic ), 2 )];
               break;

            case 'k':
               imcone_argument( CH_IMCNAME( ch ), tmp_str );
               i = ( char * )tmp_str;
               break;
            case 'K':
               imcone_argument( CH_IMCNAME( vic ), tmp_str );
               i = ( char * )tmp_str;
               break;
         }
      }
      ++format;
      while( ( *point = *i ) != '\0' )
         ++point, ++i;
   }
   *point = 0;
   point++;
   *point = '\0';

   buf[0] = UPPER( buf[0] );
   return buf;
}

CHAR_DATA *imc_make_skeleton( char *name )
{
   CHAR_DATA *skeleton;

   IMCCREATE( skeleton, CHAR_DATA, 1 );

#ifdef IMCCIRCLE
   skeleton->player.name = IMCSTRALLOC( name );
   skeleton->player.short_descr = IMCSTRALLOC( name );
   skeleton->in_room = real_room( 1 );
#else
   skeleton->name = IMCSTRALLOC( name );
   skeleton->short_descr = IMCSTRALLOC( name );
   skeleton->in_room = get_room_index( ROOM_VNUM_LIMBO );
#endif

   return skeleton;
}

void imc_purge_skeleton( CHAR_DATA * skeleton )
{
   if( !skeleton )
      return;

#ifdef IMCCIRCLE
   IMCSTRFREE( skeleton->player.name );
   IMCSTRFREE( skeleton->player.short_descr );
#else
   IMCSTRFREE( skeleton->name );
   IMCSTRFREE( skeleton->short_descr );
#endif
   IMCDISPOSE( skeleton );

   return;
}

/* Socials can now be called anywhere you want them - like for instance, tells.
 * Thanks to Darien@Sandstorm for this suggestion. -- Samson 2-21-04
 */
char *imc_send_social( CHAR_DATA * ch, char *argument, int telloption )
{
   CHAR_DATA *skeleton = NULL;
   char *ps;
   char socbuf[LGST], msg[LGST];
   char arg1[SMST], person[SMST], mud[SMST], buf[LGST];
   unsigned int x;

   person[0] = '\0';
   mud[0] = '\0';

   /*
    * Name of social, remainder of argument is assumed to hold the target
    */
   argument = imcone_argument( argument, arg1 );

   if( argument && argument[0] != '\0' )
   {
      if( !( ps = strchr( argument, '@' ) ) )
      {
         imc_to_char( "You need to specify a person@mud for a target.\r\n", ch );
         return "";
      }
      else
      {
         for( x = 0; x < strlen( argument ); x++ )
         {
            person[x] = argument[x];
            if( person[x] == '@' )
               break;
         }
         person[x] = '\0';

         ps[0] = '\0';
         imcstrlcpy( mud, ps + 1, SMST );
      }
   }

   if( telloption == 0 )
   {
      snprintf( socbuf, LGST, "%s", imc_find_social( ch, arg1, person, mud, 0 ) );
      if( socbuf[0] == '\0' )
         return "";
   }

   if( telloption == 1 )
   {
      snprintf( socbuf, LGST, "%s", imc_find_social( ch, arg1, person, mud, 1 ) );
      if( socbuf[0] == '\0' )
         return "";
   }

   if( telloption == 2 )
   {
      snprintf( socbuf, LGST, "%s", imc_find_social( ch, arg1, person, mud, 2 ) );
      if( socbuf[0] == '\0' )
         return "";
   }

   if( argument && argument[0] != '\0' )
   {
      int sex;

      snprintf( buf, LGST, "%s@%s", person, mud );
      sex = imc_get_ucache_gender( buf );
      if( sex == -1 )
      {
         imc_send_ucache_request( buf );
         sex = SEX_MALE;
      }
      else
         sex = imctodikugender( sex );

      skeleton = imc_make_skeleton( buf );
      CH_IMCSEX( skeleton ) = sex;
   }

   imcstrlcpy( msg, ( char * )imc_act_string( socbuf, ch, skeleton ), LGST );
   if( skeleton )
      imc_purge_skeleton( skeleton );
   return ( color_mtoi( msg ) );
}
#endif /* IMCSTANDALONE */

char *imc_funcname( IMC_FUN * func )
{
   if( func == imc_other )
      return ( "imc_other" );
   if( func == imclisten )
      return ( "imclisten" );
   if( func == imcchanlist )
      return ( "imcchanlist" );
   if( func == imclist )
      return ( "imclist" );
   if( func == imcinvis )
      return ( "imcinvis" );
   if( func == imcwho )
      return ( "imcwho" );
   if( func == imclocate )
      return ( "imclocate" );
   if( func == imctell )
      return ( "imctell" );
   if( func == imcreply )
      return ( "imcreply" );
   if( func == imcbeep )
      return ( "imcbeep" );
   if( func == imcignore )
      return ( "imcignore" );
   if( func == imcfinger )
      return ( "imcfinger" );
   if( func == imcinfo )
      return ( "imcinfo" );
   if( func == imccolor )
      return ( "imccolor" );
   if( func == imcafk )
      return ( "imcafk" );
   if( func == imcchanwho )
      return ( "imcchanwho" );
   if( func == imcconnect )
      return ( "imcconnect" );
   if( func == imcdisconnect )
      return ( "imcdisconnect" );
   if( func == imcpermstats )
      return ( "imcpermstats" );
   if( func == imc_deny_channel )
      return ( "imc_deny_channel" );
   if( func == imcpermset )
      return ( "imcpermset" );
   if( func == imcsetup )
      return ( "imcsetup" );
   if( func == imccommand )
      return ( "imccommand" );
   if( func == imcban )
      return ( "imcban" );
   if( func == imcconfig )
      return ( "imcconfig" );
   if( func == imc_show_ucache_contents )
      return ( "imc_show_ucache_contents" );
   if( func == imcremoteadmin )
      return ( "imcremoteadmin" );
   if( func == imcdebug )
      return ( "imcdebug" );
   if( func == imchedit )
      return ( "imchedit" );
   if( func == imchelp )
      return ( "imchelp" );
   if( func == imccedit )
      return ( "imccedit" );
   if( func == imcnotify )
      return ( "imcnotify" );
   if( func == imcrefresh )
      return ( "imcrefresh" );
   if( func == imclast )
      return ( "imclast" );
   if( func == imctemplates )
      return ( "imctemplates" );
   return "";
}

IMC_FUN *imc_function( const char *func )
{
   if( !strcasecmp( func, "imc_other" ) )
      return imc_other;
   if( !strcasecmp( func, "imclisten" ) )
      return imclisten;
   if( !strcasecmp( func, "imcchanlist" ) )
      return imcchanlist;
   if( !strcasecmp( func, "imclist" ) )
      return imclist;
   if( !strcasecmp( func, "imcinvis" ) )
      return imcinvis;
   if( !strcasecmp( func, "imcwho" ) )
      return imcwho;
   if( !strcasecmp( func, "imclocate" ) )
      return imclocate;
   if( !strcasecmp( func, "imctell" ) )
      return imctell;
   if( !strcasecmp( func, "imcreply" ) )
      return imcreply;
   if( !strcasecmp( func, "imcbeep" ) )
      return imcbeep;
   if( !strcasecmp( func, "imcignore" ) )
      return imcignore;
   if( !strcasecmp( func, "imcfinger" ) )
      return imcfinger;
   if( !strcasecmp( func, "imcinfo" ) )
      return imcinfo;
   if( !strcasecmp( func, "imccolor" ) )
      return imccolor;
   if( !strcasecmp( func, "imcafk" ) )
      return imcafk;
   if( !strcasecmp( func, "imcchanwho" ) )
      return imcchanwho;
   if( !strcasecmp( func, "imcconnect" ) )
      return imcconnect;
   if( !strcasecmp( func, "imcdisconnect" ) )
      return imcdisconnect;
   if( !strcasecmp( func, "imcpermstats" ) )
      return imcpermstats;
   if( !strcasecmp( func, "imc_deny_channel" ) )
      return imc_deny_channel;
   if( !strcasecmp( func, "imcpermset" ) )
      return imcpermset;
   if( !strcasecmp( func, "imcsetup" ) )
      return imcsetup;
   if( !strcasecmp( func, "imccommand" ) )
      return imccommand;
   if( !strcasecmp( func, "imcban" ) )
      return imcban;
   if( !strcasecmp( func, "imcconfig" ) )
      return imcconfig;
   if( !strcasecmp( func, "imc_show_ucache_contents" ) )
      return imc_show_ucache_contents;
   if( !strcasecmp( func, "imcremoteadmin" ) )
      return imcremoteadmin;
   if( !strcasecmp( func, "imcdebug" ) )
      return imcdebug;
   if( !strcasecmp( func, "imchelp" ) )
      return imchelp;
   if( !strcasecmp( func, "imccedit" ) )
      return imccedit;
   if( !strcasecmp( func, "imchedit" ) )
      return imchedit;
   if( !strcasecmp( func, "imcnotify" ) )
      return imcnotify;
   if( !strcasecmp( func, "imcrefresh" ) )
      return imcrefresh;
   if( !strcasecmp( func, "imclast" ) )
      return imclast;
   if( !strcasecmp( func, "imctemplates" ) )
      return imctemplates;

   return NULL;
}

/* Check for IMC channels, return TRUE to stop command processing, FALSE otherwise */
bool imc_command_hook( CHAR_DATA * ch, char *command, char *argument )
{
   IMC_CMD_DATA *cmd;
   IMC_ALIAS *alias;
   IMC_CHANNEL *c;
   char *p;

#if !defined(IMCSTANDALONE)
   if( IS_NPC( ch ) )
      return FALSE;
#endif

   if( !this_imcmud )
   {
      imcbug( "%s", "Ooops. IMC being called with no configuration!" );
      return FALSE;
   }

   if( !first_imc_command )
   {
      imcbug( "%s", "ACK! There's no damn command data loaded!" );
      return FALSE;
   }

   if( IMCPERM( ch ) <= IMCPERM_NONE )
      return FALSE;

#if defined(IMCCIRCLE)
   /*
    * CircleMUD parser leaves leading spaces after splitting one argument
    */
   skip_spaces( &argument );
#endif

   /*
    * Simple command interpreter menu. Nothing overly fancy etc, but it beats trying to tie directly into the mud's
    * * own internal structures. Especially with the differences in codebases.
    */
   for( cmd = first_imc_command; cmd; cmd = cmd->next )
   {
      if( IMCPERM( ch ) < cmd->level )
         continue;

      for( alias = cmd->first_alias; alias; alias = alias->next )
      {
         if( !strcasecmp( command, alias->name ) )
         {
            command = cmd->name;
            break;
         }
      }

      if( !strcasecmp( command, cmd->name ) )
      {
         if( cmd->connected == TRUE && this_imcmud->state < IMC_ONLINE )
         {
            imc_to_char( "The mud is not currently connected to IMC2.\r\n", ch );
            return TRUE;
         }

         if( cmd->function == NULL )
         {
            imc_to_char( "That command has no code set. Inform the administration.\r\n", ch );
            imcbug( "imc_command_hook: Command %s has no code set!", cmd->name );
            return TRUE;
         }

         ( *cmd->function ) ( ch, argument );
         return TRUE;
      }
   }

   /*
    * Assumed to be aiming for a channel if you get this far down
    */
   c = imc_findchannel( command );

   if( !c || c->level > IMCPERM( ch ) )
      return FALSE;

   if( imc_hasname( IMC_DENY( ch ), c->local_name ) )
   {
      imc_printf( ch, "You have been denied the use of %s by the administration.\r\n", c->local_name );
      return TRUE;
   }

   if( !c->refreshed )
   {
      imc_printf( ch, "The %s channel has not yet been refreshed by the server.\r\n", c->local_name );
      return TRUE;
   }

   if( !argument || argument[0] == '\0' )
   {
      int y;

      imc_printf( ch, "~cThe last %d %s messages:\r\n", MAX_IMCHISTORY, c->local_name );
      for( y = 0; y < MAX_IMCHISTORY; y++ )
      {
         if( c->history[y] != NULL )
            imc_printf( ch, "%s\r\n", c->history[y] );
         else
            break;
      }
      return TRUE;
   }

   if( IMCPERM( ch ) >= IMCPERM_ADMIN && !strcasecmp( argument, "log" ) )
   {
      if( !IMCIS_SET( c->flags, IMCCHAN_LOG ) )
      {
         IMCSET_BIT( c->flags, IMCCHAN_LOG );
         imc_printf( ch, "~RFile logging enabled for %s, PLEASE don't forget to undo this when it isn't needed!\r\n",
                     c->local_name );
      }
      else
      {
         IMCREMOVE_BIT( c->flags, IMCCHAN_LOG );
         imc_printf( ch, "~GFile logging disabled for %s.\r\n", c->local_name );
      }
      imc_save_channels(  );
      return TRUE;
   }

   if( !imc_hasname( IMC_LISTEN( ch ), c->local_name ) )
   {
      imc_printf( ch, "You are not currently listening to %s. Use the imclisten command to listen to this channel.\r\n",
                  c->local_name );
      return TRUE;
   }

   switch ( argument[0] )
   {
      case ',':
         /*
          * Strip the , and then extra spaces - Remcon 6-28-03
          */
         argument++;
         while( isspace( *argument ) )
            argument++;
         imc_sendmessage( c, CH_IMCNAME( ch ), color_mtoi( argument ), 1 );
         break;
      case '@':
         /*
          * Strip the @ and then extra spaces - Remcon 6-28-03
          */
         argument++;
         while( isspace( *argument ) )
            argument++;
         p = imc_send_social( ch, argument, 0 );
         if( !p || p[0] == '\0' )
            return TRUE;
         imc_sendmessage( c, CH_IMCNAME( ch ), p, 2 );
         break;
      default:
         imc_sendmessage( c, CH_IMCNAME( ch ), color_mtoi( argument ), 0 );
         break;
   }
   return TRUE;
}

#if defined(IMCSTANDALONE)
int main( int argc, char **argv )
{
   bool client_down = FALSE;

   first_descriptor = last_descriptor = NULL;

   imclog( "IMC2 Standalone connection startup: %s", IMC_VERSION_STRING );
   imc_startup( FALSE, -1, FALSE );
   while( !client_down )
   {
      imc_loop();
      usleep( 250000 );
   }
   imclog( "%s", "Something caused me to close!" );
   exit(0);
   return 0;
}
#endif
