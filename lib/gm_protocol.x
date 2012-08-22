#define __GANGLIA_MTU     1500
#define __UDP_HEADER_SIZE 28
#define __MAX_TITLE_LEN 32
#define __MAX_GROUPS_LEN 128
#define __MAX_DESC_LEN 128
%#define UDP_HEADER_SIZE __UDP_HEADER_SIZE
%#define MAX_DESC_LEN __MAX_DESC_LEN
%#define GM_PROTOCOL_GUARD

#define GANGLIA_MAX_XDR_MESSAGE_LEN (__GANGLIA_MTU - __UDP_HEADER_SIZE - 8)

enum Ganglia_value_types {
  GANGLIA_VALUE_UNKNOWN,
  GANGLIA_VALUE_STRING, 
  GANGLIA_VALUE_UNSIGNED_SHORT,
  GANGLIA_VALUE_SHORT,
  GANGLIA_VALUE_UNSIGNED_INT,
  GANGLIA_VALUE_INT,
  GANGLIA_VALUE_FLOAT,
  GANGLIA_VALUE_DOUBLE
};

/* Structure for transporting extra metadata */
struct Ganglia_extra_data {
  string name<>;
  string data<>;
};

struct Ganglia_metadata_message {
  string type<>;
  string name<>;
  string units<>;
  unsigned int slope;
  unsigned int tmax;
  unsigned int dmax;
  struct Ganglia_extra_data metadata<>;
};

struct Ganglia_metric_id {
  string host<>;
  string name<>;
  bool spoof;
};

struct Ganglia_uuid {
  unsigned char uuid[16];
};

struct Ganglia_metadatadef {
  struct Ganglia_metric_id metric_id;
  struct Ganglia_metadata_message metric;
};

struct Ganglia_metadatadef_with_uuid {
  struct Ganglia_metric_id metric_id;
  struct Ganglia_metadata_message metric;
  Ganglia_uuid uuid;
};

struct Ganglia_metadatareq {
  struct Ganglia_metric_id metric_id;
};

struct Ganglia_gmetric_ushort {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  unsigned short us;
};

struct Ganglia_gmetric_short {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  short ss;
};

struct Ganglia_gmetric_int {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  int si;  
};

struct Ganglia_gmetric_uint {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  unsigned int ui;  
};

struct Ganglia_gmetric_string {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  string str<>;
};

struct Ganglia_gmetric_float {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  float f;
};

struct Ganglia_gmetric_double {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  double d;
};

struct Ganglia_gmetric_ushort_with_uuid {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  unsigned short us;
  Ganglia_uuid uuid;
};

struct Ganglia_gmetric_short_with_uuid {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  short ss;
  Ganglia_uuid uuid;
};

struct Ganglia_gmetric_int_with_uuid {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  int si;
  Ganglia_uuid uuid;
};

struct Ganglia_gmetric_uint_with_uuid {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  unsigned int ui;
  Ganglia_uuid uuid;
};

struct Ganglia_gmetric_string_with_uuid {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  string str<>;
  Ganglia_uuid uuid;
};

struct Ganglia_gmetric_float_with_uuid {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  float f;
  Ganglia_uuid uuid;
};

struct Ganglia_gmetric_double_with_uuid {
  struct Ganglia_metric_id metric_id;
  string fmt<>;
  double d;
  Ganglia_uuid uuid;
};

/* Start the refactored XDR packet ids at 128 to 
** avoid confusing the new packets with the older ones.
** this is to avoid trying to decode older XDR packets
** as newer packets if an older version of gmond happens
** to be talking on the same multicast channel.
*/
enum Ganglia_msg_formats {
   gmetadata_full = 128,
   gmetric_ushort,
   gmetric_short,
   gmetric_int,
   gmetric_uint,
   gmetric_string,
   gmetric_float,
   gmetric_double,
   gmetadata_request,
   gmetadata_full_with_uuid = 192,
   gmetric_ushort_with_uuid,
   gmetric_short_with_uuid,
   gmetric_int_with_uuid,
   gmetric_uint_with_uuid,
   gmetric_string_with_uuid,
   gmetric_float_with_uuid,
   gmetric_double_with_uuid
};

union Ganglia_metadata_msg switch (Ganglia_msg_formats id) {
  case gmetadata_full:
    Ganglia_metadatadef gfull;
  case gmetadata_full_with_uuid:
    Ganglia_metadatadef_with_uuid gfull_u;
  case gmetadata_request:
    Ganglia_metadatareq grequest;

  default:
    void;
};

union Ganglia_value_msg switch (Ganglia_msg_formats id) {
  case gmetric_ushort:
    Ganglia_gmetric_ushort gu_short;
  case gmetric_short:
    Ganglia_gmetric_short gs_short;
  case gmetric_int:
    Ganglia_gmetric_int gs_int;
  case gmetric_uint:
    Ganglia_gmetric_uint gu_int;
  case gmetric_string:
    Ganglia_gmetric_string gstr;
  case gmetric_float:
    Ganglia_gmetric_float gf;
  case gmetric_double:
    Ganglia_gmetric_double gd;

  case gmetric_ushort_with_uuid:
    Ganglia_gmetric_ushort_with_uuid gu_short_u;
  case gmetric_short_with_uuid:
    Ganglia_gmetric_short_with_uuid gs_short_u;
  case gmetric_int_with_uuid:
    Ganglia_gmetric_int_with_uuid gs_int_u;
  case gmetric_uint_with_uuid:
    Ganglia_gmetric_uint_with_uuid gu_int_u;
  case gmetric_string_with_uuid:
    Ganglia_gmetric_string_with_uuid gstr_u;
  case gmetric_float_with_uuid:
    Ganglia_gmetric_float_with_uuid gf_u;
  case gmetric_double_with_uuid:
    Ganglia_gmetric_double_with_uuid gd_u;

  default:
    void;
};

struct Ganglia_25metric
{
   int key;
   string name<16>;
   int tmax;
   Ganglia_value_types type;
   string units<32>;
   string slope<32>;
   string fmt<32>;
   int msg_size;
   string desc<__MAX_DESC_LEN>;
   int *metadata;
};

#ifdef RPC_HDR
% #define GANGLIA_MAX_MESSAGE_LEN GANGLIA_MAX_XDR_MESSAGE_LEN

/* This is a place holder for the key field
 *  in the Ganglia_25metric structure. For now the only
 *  type of metric is modular.
 */
%#define modular_metric 4098
#endif

