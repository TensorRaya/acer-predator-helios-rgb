#!/usr/bin/env python3
"""USBPcap (.pcap, linktype 249) parsen: alle Host->Device Control(SET_REPORT)- und Interrupt-OUT-Transfers ausgeben."""
import struct, sys, collections
def parse(path):
    f=open(path,"rb").read()
    magic,=struct.unpack("<I",f[:4]); assert magic==0xa1b2c3d4
    linktype,=struct.unpack("<I",f[20:24]); off=24; out=[]
    while off+16<=len(f):
        ts,tus,incl,orig=struct.unpack("<IIII",f[off:off+16]); off+=16
        pkt=f[off:off+incl]; off+=incl
        if len(pkt)<27: continue
        hl,irp,status,func,info,bus,dev,ep,tt,dlen=struct.unpack("<HQIHBHHBBI",pkt[:27])
        payload=pkt[hl:hl+dlen]
        direction = "IN" if ep & 0x80 else "OUT"
        if tt==2:  # control
            stage=pkt[27] if hl>27 else -1
            if func==0x0008 and stage==0 and dlen>=8:  # URB_FUNCTION_CONTROL_TRANSFER setup stage
                setup=payload[:8]; data=payload[8:]
                bm,breq,wval,widx,wlen=struct.unpack("<BBHHH",setup)
                if not (bm & 0x80): out.append(("CTRL",f"bm={bm:02x} req={breq} val={wval:04x} idx={widx} len={wlen}",data))
        elif tt==1 and direction=="OUT" and dlen>0:  # interrupt out
            out.append(("INTR",f"ep={ep:02x}",payload))
    return out
for p in sys.argv[1:]:
    print("=====",p)
    seq=parse(p); print("transfers:",len(seq))
    last=None; rep=0
    for kind,meta,data in seq:
        line=(kind,meta,data.hex())
        if line==last: rep+=1; continue
        if rep: print(f"   ... x{rep+1}"); rep=0
        last=line
        h=data.hex()
        print(f"{kind} {meta} :: {h[:96]}{'…' if len(h)>96 else ''} ({len(data)}B)")
    if rep: print(f"   ... x{rep+1}")
