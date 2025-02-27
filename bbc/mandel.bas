10REM https://projects.drogon.net/retro-basic-benchmarking-with-mandelbrot/
100PRINT "Mandelbrot - BBC Basic - FP + Strings"
110PRINT "Start"
120TIME=0:REM Initialize TIME
130Z$=".,'~=+:;*%&$OXB#@ "
140F=50
150FOR Y = -12 TO 12
160FOR X = -49 TO 29
170C=X*229/100
180D=Y*416/100
190A=C:B=D:I=0
200Q=B/F:S=B-(Q*F)
210T=((A*A)-(B*B))/F+C
220B=2*((A*Q)+(A*S/F))+D
230A=T:P=A/F:Q=B/F
240IF ((P*P)+(Q*Q))>=5 GOTO 280
250I=I+1:IF I<16 GOTO 200
260PRINT " ";
270GOTO 290
280PRINT MID$(Z$,I+1,1);
290NEXT X
300PRINT ""
310NEXT Y
320Q=TIME
330PRINT "Finished"
340PRINT "Time: ";Q/100;" secs."
