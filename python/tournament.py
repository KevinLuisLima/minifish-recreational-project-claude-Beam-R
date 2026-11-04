import argparse,subprocess,sys
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--engine",default="./chess_engine")
    ap.add_argument("--models-dir",default="models")
    ap.add_argument("--games",type=int,default=4)
    args=ap.parse_args()
    cmd=[args.engine,"--tournament",args.models_dir,str(args.games)]
    r=subprocess.run(cmd,capture_output=True,text=True,timeout=600)
    print(r.stdout)
    if r.stderr: print(r.stderr,file=sys.stderr)
if __name__=="__main__":
    main()
EOF