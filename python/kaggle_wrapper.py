import sys,subprocess
ENGINE="./chess_engine"
def move(fen, weights="models/model0.bin"):
    try:
        r=subprocess.run([ENGINE, fen, "--weights", weights, "--depth", "3", "--time", "2.0"], capture_output=True, text=True, timeout=10)
        out=r.stdout.strip().splitlines()
        if out:
            return out[-1].strip()
    except Exception:
        pass
    # contingency legal first move via engine default
    try:
        r=subprocess.run([ENGINE, fen], capture_output=True, text=True, timeout=10)
        out=r.stdout.strip().splitlines()
        if out:
            return out[-1].strip()
    except Exception:
        pass
    return "e2e4"
if __name__=="__main__":
    fen=" ".join(sys.argv[1:]) if len(sys.argv)>1 else "startpos"
    print(move(fen))