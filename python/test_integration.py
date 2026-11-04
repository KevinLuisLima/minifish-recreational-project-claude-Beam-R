import subprocess,os,sys
engine="./chess_engine"
# basic move
fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
r=subprocess.run([engine,fen],capture_output=True,text=True)
print("move:",r.stdout.strip())
# perft
r2=subprocess.run([engine,"--perft","5"],capture_output=True,text=True)
print("perft5:",r2.stdout.strip())
assert r2.stdout.strip()=="4865609"
# wrapper
print("integration OK")
