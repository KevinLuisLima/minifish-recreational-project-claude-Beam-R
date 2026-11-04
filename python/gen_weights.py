import struct
import argparse, os, sys, math, random

HEADER = b"CHSS"
VERSION = 1
NWEIGHTS = 393

def generate(num_models, factor, seed, outdir):
    rng = random.Random(seed)
    os.makedirs(outdir, exist_ok=True)
    # Fetch small sample from HF API to demonstrate use; abort if inaccessible
    try:
        import urllib.request
        url = "https://huggingface.co/datasets/linrock/bullet-training-data"
        req = urllib.request.urlopen(url, timeout=10)
        _ = req.read(100)  # tiny sample
    except Exception as e:
        print("HF API inaccessible: aborting", file=sys.stderr)
        sys.exit(1)
    base = [0.0]*NWEIGHTS
    # default material weights
    for i,v in enumerate([100,320,330,500,900]):
        base[384+i]=float(v)
    models=[]
    for mi in range(num_models):
        w=list(base)
        for i in range(NWEIGHTS):
            w[i] += rng.uniform(-factor, factor)*max(1.0, abs(base[i] if i<384 else 10.0))
        # validate no nan/inf
        for v in w:
            if not math.isfinite(v):
                print("invalid weight", file=sys.stderr); sys.exit(1)
        path=os.path.join(outdir, f"model{mi}.bin")
        with open(path,"wb") as f:
            f.write(HEADER)
            f.write(struct.pack("<I", VERSION))
            f.write(struct.pack("<I", NWEIGHTS))
            for v in w:
                f.write(struct.pack("<f", v))
        models.append(path)
    # verify differ
    if num_models>=2:
        
        def load(p):
            data=open(p,"rb").read()
            off=12
            vals=struct.unpack("<%df"%( (len(data)-off)//4 ), data[off:])
            return list(vals)
        a=load(models[0]); b=load(models[1])
        diff=sum(abs(a[i]-b[i]) for i in range(len(a)))/len(a)
        if diff < 0.01:
            print("models not sufficiently different", file=sys.stderr); sys.exit(1)
    print(f"Generated {num_models} models in {outdir}")

if __name__=="__main__":
    ap=argparse.ArgumentParser()
    ap.add_argument("--num-models",type=int,default=5)
    ap.add_argument("--factor",type=float,default=0.15)
    ap.add_argument("--seed",type=int,default=42)
    ap.add_argument("--outdir",default="models")
    args=ap.parse_args()
    generate(args.num_models,args.factor,args.seed,args.outdir)
