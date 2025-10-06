from subprocess import*
import time
a=Popen("/kaggle_simulations/agent/a",stdin=PIPE,stdout=PIPE,text=True,bufsize=1)
f=True
def c(o):
    try:
        global f
        t1=time.time()
        print(o.remainingOverageTime)
        if f:
            t="f"+o.board
            f=False
        else:
            t=chr(max(32,int(o.remainingOverageTime)+31))+o.lastMove
        print("Writing:"+t)
        a.stdin.write(t)
        a.stdin.flush()
        while l:=a.stdout.readline():
            if(l[0]=="M"):
                print(l)
            else:
                print(time.time()-t1)
                return l.strip()
    except Exception as e:
        print(e,flush=True)
        return ""