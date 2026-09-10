#!/usr/bin/env python3
# Interaktives Keymap-Mapping: Zelle i weiss, auf Tastendruck warten (evdev, gegrabbt), Code speichern. Timeout = keine Taste.
import sys, os, struct, select, fcntl, json, time, re, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__))); import predator_rgb as p
OUT=os.environ.get("KEYMAP_OUT", "keymap.json"); TIMEOUT=8.0
EVIOCGRAB=0x40044590; EV_KEY=1
names={}
for h in ["/usr/include/linux/input-event-codes.h"]:
    if os.path.exists(h):
        for m in re.finditer(r"#define\s+(KEY_\w+)\s+(0x[0-9a-fA-F]+|\d+)", open(h).read()):
            names.setdefault(int(m.group(2),0), m.group(1))
# keyboards: devices with EV_KEY and KEY_A capability
fds=[]
for ev in glob.glob("/dev/input/event*"):
    try:
        caps=open(os.path.join("/sys/class/input", os.path.basename(ev), "device/capabilities/key")).read().split()
        bits=int("".join(f"{int(w,16):016x}" for w in caps),16)
        if not (bits>>30)&1: continue          # KEY_A = 30
        fd=os.open(ev, os.O_RDONLY|os.O_NONBLOCK); fcntl.ioctl(fd, EVIOCGRAB, 1); fds.append(fd)
        print("grab", ev, open(os.path.join("/sys/class/input", os.path.basename(ev), "device/name")).read().strip(), flush=True)
    except Exception as e: pass
kb=p.HidRaw(p.find_hidraw(0x05AF,0x667B,3))
res=json.load(open(OUT)) if os.path.exists(OUT) else {}
def drain(t):
    end=time.time()+t
    while time.time()<end:
        r,_,_=select.select(fds,[],[],0.05)
        for fd in r:
            try: os.read(fd,24*64)
            except BlockingIOError: pass
def light(i):
    cells=[(0,0,0)]*128; cells[i]=(255,255,255); p.vendor_perkey(kb,cells,100)
    drain(0.6)   # der MCU feuert nach jedem Frame ein Phantom-Event (LEFTCTRL) -> verwerfen
def wait_key(t):
    end=time.time()+t
    while time.time()<end:
        r,_,_=select.select(fds,[],[],0.05)
        for fd in r:
            try: data=os.read(fd,24*64)
            except BlockingIOError: continue
            for off in range(0,len(data)-23,24):
                sec,usec,typ,code,val=struct.unpack("llHHi",data[off:off+24])
                if typ==EV_KEY and val==1: return code
    return None
try:
    for i in range(128):
        if str(i) in res: continue
        light(i); code=wait_key(TIMEOUT)
        res[str(i)]= {"code":code,"name":names.get(code,str(code))} if code is not None else None
        json.dump(res,open(OUT,"w"))
        print(f"cell {i:3d} -> {res[str(i)]['name'] if res[str(i)] else '-'}", flush=True)
        time.sleep(0.3); wait_key(0.2)  # Release/Nachschwinger schlucken
finally:
    for fd in fds: fcntl.ioctl(fd, EVIOCGRAB, 0); os.close(fd)
    kb.close()
print("FERTIG", sum(1 for v in res.values() if v), "Tasten")
