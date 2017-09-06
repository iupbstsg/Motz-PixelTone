'use strict';
const BlinkyBox=(c,timeout,delay)=>{
    let running=true,

        gray=()=>{
            if(running){
                setTimeout(white,timeout);
                c.fillStyle='hsl(0,0%,30%)';
                c.fillRect(0,0,c.canvas.clientWidth,200);
            }
        },

        white=()=>{
            if(running){
                setTimeout(gray,1000/120);
                c.fillStyle='hsl(40,10%,90%)';
                c.fillRect(0,0,c.canvas.clientWidth,200);
            }
        },

        start=()=>{
            setTimeout(gray,delay);
        },

        toggle=()=>{
            if(running){running=false;}
            else{running=true; start();}
        };

    return({start,toggle});
};

const c1=document.getElementById('c1').getContext('2d'),
      c2=document.getElementById('c2').getContext('2d'),
      a=BlinkyBox(c1,1000,0),
      b=BlinkyBox(c2,2000,500);

window.onload=()=>{
    c1.canvas.width=c1.canvas.clientWidth;
    c1.canvas.height=c1.canvas.clientHeight;
    c2.canvas.width=c2.canvas.clientWidth;
    c2.canvas.height=c2.canvas.clientHeight;

    document.onclick=()=>{
        a.toggle();
        b.toggle();
    }
    //c1.canvas.onclick=(e)=>{
    //    a.toggle();
    //};
    //c2.canvas.onclick=(e)=>{
    //    b.toggle();
    //};

    a.start();
    b.start();
};

