#include "gcs_link.h"
#include "gcs_format.h"
#define snprintf gcs_snprintf
#include <string.h>
#include <math.h>

#define TIMEOUT_MS 600u
#define RX_SIZE 1024u
static bool armed, estop_latched, configured, heartbeat_seen, session_seen, outputs_known;
static uint16_t minimum_us, maximum_us, ramp_rate, target[4], applied[4];
static uint32_t last_heartbeat, last_status, last_ramp, ramp_fraction, tx_seq, rx_seq;
static gcs_health_t imu_health, baro_health;
static gcs_calibration_t calibration_state;
static char rx[RX_SIZE];
static size_t rx_len;
static bool discarding;

static uint16_t crc16(const char *s,size_t n) {
    uint16_t crc=0xffff;
    while(n--) {
        crc^=(uint16_t)(uint8_t)*s++<<8;
        for(unsigned i=0;i<8;i++) crc=(uint16_t)((crc&0x8000)?(crc<<1)^0x1021:crc<<1);
    }
    return crc;
}
static void emit(const char *payload) {
    char out[1024];
    int n=snprintf(out,sizeof out,"{\"v\":2,\"seq\":%lu,\"payload\":%s",(unsigned long)++tx_seq,payload);
    if(n<=0||(size_t)n>=sizeof out) return;
    uint16_t crc=crc16(out,(size_t)n);
    int tail=snprintf(out+n,sizeof out-(size_t)n,",\"crc\":%u}\n",(unsigned)crc);
    if(tail>0&&(size_t)tail<sizeof out-(size_t)n) gcs_uart_send(out,(size_t)(n+tail));
}
static void status(const char *reason) {
    char out[768];
    int n=snprintf(out,sizeof out,
        "{\"type\":\"status\",\"armed\":%s,\"estop\":%s,\"reason\":\"%s\","
        "\"bench\":%s,\"configured\":%s,\"outputs_known\":%s,\"min_us\":%u,\"max_us\":%u,\"ramp\":%u,"
        "\"motors\":[{\"requested\":%u,\"applied\":%u},{\"requested\":%u,\"applied\":%u},"
        "{\"requested\":%u,\"applied\":%u},{\"requested\":%u,\"applied\":%u}]}",
        armed?"true":"false",estop_latched?"true":"false",reason,
        gcs_physical_bench_enable()?"true":"false",configured?"true":"false",outputs_known?"true":"false",
        minimum_us,maximum_us,ramp_rate,target[0],applied[0],target[1],applied[1],
        target[2],applied[2],target[3],applied[3]);
    if(n>0&&(size_t)n<sizeof out) emit(out);
}
static void stop_outputs(void) {
    armed=false;
    memset(target,0,sizeof target);
    outputs_known=gcs_esc_disable_outputs();
    if(outputs_known) memset(applied,0,sizeof applied);
    else estop_latched=true;
}
void gcs_transport_fault(void) {
    stop_outputs(); session_seen=false; heartbeat_seen=false;
    rx_len=0; discarding=true;
}
static void ack(uint32_t seq,const char *command,bool ok,const char *reason,unsigned id) {
    char out[320];
    unsigned actual=(id>=1&&id<=4&&outputs_known)?applied[id-1]:0;
    int n=snprintf(out,sizeof out,
        "{\"type\":\"ack\",\"command_seq\":%lu,\"command\":\"%s\",\"ok\":%s,\"reason\":\"%s\","
        "\"armed\":%s,\"estop\":%s,\"id\":%u,\"applied\":%u}",
        (unsigned long)seq,command,ok?"true":"false",reason,armed?"true":"false",estop_latched?"true":"false",id,actual);
    if(n>0&&(size_t)n<sizeof out) emit(out);
}
void gcs_init(uint32_t now_ms) {
    armed=estop_latched=configured=heartbeat_seen=session_seen=false;
    minimum_us=maximum_us=1000; ramp_rate=200;
    rx_len=0; discarding=false; tx_seq=rx_seq=0;
    last_heartbeat=last_status=last_ramp=now_ms; ramp_fraction=0;
    imu_health=baro_health=GCS_HEALTH_UNKNOWN; calibration_state=GCS_CAL_UNKNOWN;
    stop_outputs();
}
bool gcs_configure_esc(uint16_t min_us,uint16_t max_us,uint16_t rate) {
    if(armed||min_us<1000||max_us>2000||min_us>=max_us||rate<50||rate>5000) return false;
    minimum_us=min_us; maximum_us=max_us; ramp_rate=rate; configured=true; return true;
}
void gcs_set_sensor_health(gcs_health_t imu,gcs_health_t barometer,gcs_calibration_t calibration) {
    imu_health=((unsigned)imu<=GCS_HEALTH_FAULT)?imu:GCS_HEALTH_UNKNOWN;
    baro_health=((unsigned)barometer<=GCS_HEALTH_FAULT)?barometer:GCS_HEALTH_UNKNOWN;
    calibration_state=((unsigned)calibration<=GCS_CAL_DONE)?calibration:GCS_CAL_UNKNOWN;
}
static bool parse_uint(const char *s,size_t n,uint32_t *value) {
    if(n==0||n>10) return false;
    uint32_t v=0;
    for(size_t i=0;i<n;i++) {
        if(s[i]<'0'||s[i]>'9') return false;
        unsigned d=(unsigned)(s[i]-'0');
        if(v>(UINT32_MAX-d)/10u) return false;
        v=v*10u+d;
    }
    *value=v; return true;
}
static bool unwrap(char *line,uint32_t *sequence,char **payload) {
    const char *prefix="{\"v\":2,\"seq\":";
    if(strncmp(line,prefix,strlen(prefix))) return false;
    char *body=strstr(line+strlen(prefix),",\"payload\":");
    char *crc=strstr(line,",\"crc\":");
    size_t length=strlen(line);
    if(!body||!crc||crc<body||length==0||line[length-1]!='}') return false;
    if(!parse_uint(line+strlen(prefix),(size_t)(body-line-strlen(prefix)),sequence)) return false;
    uint32_t expected;
    if(!parse_uint(crc+7,(size_t)((line+length-1)-(crc+7)),&expected)||expected>65535) return false;
    if(crc16(line,(size_t)(crc-line))!=(uint16_t)expected) return false;
    *crc=0; *payload=body+strlen(",\"payload\":"); return true;
}
static bool parse_motor(const char *line,uint32_t *id,uint32_t *us) {
    const char *prefix="{\"type\":\"motor\",\"id\":";
    if(strncmp(line,prefix,strlen(prefix))) return false;
    const char *comma=strstr(line+strlen(prefix),",\"us\":");
    size_t len=strlen(line);
    if(!comma||!len||line[len-1]!='}') return false;
    return parse_uint(line+strlen(prefix),(size_t)(comma-line-strlen(prefix)),id)&&
           parse_uint(comma+6,(size_t)(line+len-1-(comma+6)),us);
}
static void process(char *line,uint32_t now_ms) {
    /* Unframed legacy stops are fail-closed compatibility commands, never ARM/PWM. */
    if(!strcmp(line,"{\"type\":\"estop\"}")||!strcmp(line,"{\"type\":\"disarm\"}")) {
        if(strstr(line,"estop")) estop_latched=true;
        stop_outputs(); status("legacy_stop"); return;
    }
    uint32_t seq;
    char *payload;
    if(!unwrap(line,&seq,&payload)) { status("bad_frame_or_crc"); return; }
    bool is_stop=!strcmp(payload,"{\"type\":\"estop\"}");
    bool is_disarm=!strcmp(payload,"{\"type\":\"disarm\"}");
    if(is_stop||is_disarm) {
        if(is_stop) estop_latched=true;
        stop_outputs();
        if((int32_t)(seq-rx_seq)>0) rx_seq=seq;
        ack(seq,is_stop?"estop":"disarm",outputs_known,outputs_known?"outputs_disabled":"disable_failed",0);
        status(is_stop?"estop_latched":"disarmed"); return;
    }
    if(!strcmp(payload,"{\"type\":\"hello\"}")) {
        stop_outputs(); rx_seq=seq; session_seen=true; heartbeat_seen=false;
        ack(seq,"hello",outputs_known,"protocol_2",0); status("ready"); return;
    }
    uint32_t id=0,us=0;
    bool motor=parse_motor(payload,&id,&us);
    const char *command=motor?"motor":!strcmp(payload,"{\"type\":\"arm\",\"bench\":true}")?"arm":
        !strcmp(payload,"{\"type\":\"heartbeat\"}")?"heartbeat":NULL;
    if(!command) { status("invalid_command"); return; }
    if(!session_seen||(int32_t)(seq-rx_seq)<=0) {
        ack(seq,command,false,"session_or_sequence",motor&&id>=1&&id<=4?(unsigned)id:1); return;
    }
    rx_seq=seq;
    if(!strcmp(command,"heartbeat")) {
        heartbeat_seen=true; last_heartbeat=now_ms;
        ack(seq,command,true,"heartbeat",0); return;
    }
    const char *denied=estop_latched?"estop_latched":!configured?"esc_not_configured":
        !gcs_physical_bench_enable()?"physical_interlock_open":
        (!heartbeat_seen||(uint32_t)(now_ms-last_heartbeat)>TIMEOUT_MS)?"heartbeat_timeout":NULL;
    if(!strcmp(command,"arm")) {
        stop_outputs();
        if(!denied&&!estop_latched&&outputs_known) {
            armed=true; last_ramp=now_ms; ramp_fraction=0;
            ack(seq,command,true,"armed_bench",0); status("armed_bench");
        } else {
            ack(seq,command,false,denied?denied:"disable_failed",0); status(denied?denied:"disable_failed");
        }
        return;
    }
    if(id<1||id>4) { ack(seq,command,false,"invalid_motor_id",1); return; }
    if(denied||!armed) { stop_outputs(); ack(seq,command,false,denied?denied:"not_armed",(unsigned)id); return; }
    if(us<minimum_us||us>maximum_us) { ack(seq,command,false,"pwm_out_of_range",(unsigned)id); return; }
    unsigned index=(unsigned)id-1;
    if(applied[index]==0) {
        if(!gcs_esc_write_us((uint8_t)index,minimum_us)) {
            stop_outputs(); ack(seq,command,false,"driver_write_failed",(unsigned)id); status("driver_write_failed"); return;
        }
        applied[index]=minimum_us;
    }
    target[index]=(uint16_t)us;
    ack(seq,command,true,"target_accepted",(unsigned)id);
}
void gcs_rx_byte(uint8_t byte,uint32_t now_ms) {
    if(byte=='\n') {
        if(discarding) status("invalid_frame");
        else if(rx_len) {
            if(rx[rx_len-1]=='\r') rx_len--;
            if(rx_len) { rx[rx_len]=0; process(rx,now_ms); }
        }
        rx_len=0; discarding=false; return;
    }
    if(discarding) return;
    if((byte<32&&byte!='\r')||byte>126||rx_len>=RX_SIZE-1) {
        rx_len=0; discarding=true; return;
    }
    rx[rx_len++]=(char)byte;
}
void gcs_poll(uint32_t now_ms) {
    if(armed&&((uint32_t)(now_ms-last_heartbeat)>TIMEOUT_MS||!gcs_physical_bench_enable())) {
        stop_outputs(); status("failsafe_timeout_or_interlock");
    }
    uint32_t elapsed=(uint32_t)(now_ms-last_ramp);
    if(elapsed>=10) {
        last_ramp=now_ms;
        if(elapsed>20) elapsed=20; /* A delayed task cannot cause a large ramp jump. */
        uint32_t budget=elapsed*ramp_rate+ramp_fraction;
        unsigned step=(unsigned)(budget/1000); ramp_fraction=budget%1000;
        if(armed&&step) for(unsigned i=0;i<4;i++) {
            if(!target[i]||target[i]==applied[i]) continue;
            unsigned next=applied[i];
            if(target[i]>next) next+=(target[i]-next<step)?target[i]-next:step;
            else next-=(next-target[i]<step)?next-target[i]:step;
            if(!gcs_esc_write_us((uint8_t)i,(uint16_t)next)) {
                stop_outputs(); status("driver_write_failed"); break;
            }
            applied[i]=(uint16_t)next;
        }
    }
    if((uint32_t)(now_ms-last_status)>=200) { last_status=now_ms; status("periodic"); }
}
void gcs_publish_telemetry(const gcs_telemetry_t *t) {
    if(!t) return;
    /* The STM32 port sends JSON null for unavailable/stale measurements. */
    const char *health[]={"unknown","ok","fault"};
    const char *cal[]={"unknown","needed","running","calibrated"};
    char out[768];
    int n=snprintf(out,sizeof out,
        "{\"type\":\"telemetry\",\"roll\":%.2f,\"pitch\":%.2f,\"yaw\":%.2f,"
        "\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f,"
        "\"pressure\":%.2f,\"temperature\":%.2f,\"altitude\":%.2f,"
        "\"health\":{\"imu\":\"%s\",\"barometer\":\"%s\",\"calibration\":\"%s\"}}",
        t->roll,t->pitch,t->yaw,t->ax,t->ay,t->az,t->gx,t->gy,t->gz,t->pressure,t->temperature,t->altitude,
        health[imu_health],health[baro_health],cal[calibration_state]);
    if(n>0&&(size_t)n<sizeof out) emit(out);
}
