#!/usr/bin/python
"""\
Simple g-code streaming script
no olvide instalar pyserial antes
pip install pyserial
"""
import sys
sys.path.insert(0, u'/../../../python2.7/site-packages')
import serial
import time
import argparse

parser = argparse.ArgumentParser(description='This is a basic gcode sender. Basado en  http://crcibernetica.com Modificado por Francisco Calderon')
parser.add_argument('-p','--port',help='Input USB port',required=True)
parser.add_argument('-f','--file',help='Gcode file name',required=True)
args = parser.parse_args()
 
## show values ##
print ("USB Port: %s" % args.port )
print ("Gcode file: %s" % args.file )
longstring = """\
             .,-:;//;:=,
         . :H@@@MM@M#H/.,+%;,
      ,/X+ +M@@M@MM%=,-%HMMM@X/,
     -+@MM; $M@@MH+-,;XMMMM@MMMM@+-
    ;@M@@M- XM@X;. -+XXXXXHHH@M@M#@/.
  ,%MM@@MH ,@%=            .---=-=:=,.
  -@#@@@MX .,              -%HX$$%%%+;
 =-./@M@M$                  .;@MMMM@MM:
 X@/ -$MM/                    .+MM@@@M$
,@M@H: :@:  CDIO 2 2017 - 2   . -X#@@@@-
,@@@MMX, .  Francisco         /H- ;@M@M=
.H@@@@M@+,         Calderon   %MM+..%#$.
 /MMMM@MMH/.                  XM@MH; -;
  /%+%$XHH@$=              , .H@@@@MX,
   .=--------.           -%H.,@@@@@MX,
   .%MM@@@HHHXX$$$%+- .:$MMX -M@@MM%.
     =XMMM@MM@MM#H;,-+HMM@M+ /MMMX=
       =%@M@M#@$-.=$@MM@@@M; %M%=
         ,:+$+-,/H#MMMMMMM@- -,
               =++%%%%+/:-.
"""
print longstring
print ("USB Port: %s" % args.port )
print ("Gcode file: %s" % args.file )


def removeComment(string):
	if (string.find(';')==-1):
		return string
	else:
		return string[:string.index('(')]
 
# Open serial port
#s = serial.Serial('/dev/ttyACM0',9600)#linux :)
s = serial.Serial(args.port,115200)#Guindows :(
print 'Opening Serial Port'
 
# Open g-code file
f = open(args.file,'r');
print 'Opening gcode file'+args.file
 
print '-----------------------echo-----------------------------'
character = s.read(1)
while (character != '>'):
	#print character
	sys.stdout.write(character)
	character = s.read(1)
print '---------------------fin echo---------------------------'
	
#print character
sys.stdout.write(character)
# Wake up 
s.write("\r\n\r\n") # Hit enter a few times to wake the Printrbot
time.sleep(5)   # Wait for Printrbot to initialize
s.flushInput()  # Flush startup text in serial input
print '     >    >  > >>>  Enviando GCODE <<< <  <    <     <'
lineNumber = 1;
# Stream g-code
for line in f:
	l = removeComment(line)
	l = l.strip() # Strip all EOL characters for streaming
	if  (l.isspace()==False and len(l)>0) :
		print '>     >    >  > >>> Enviando: ' + l +' esperando echo...'
		s.write(l + '\n') # le aniado /n ya que con el strip le quite todos menos este.
		print '-----> Linea de codigo: ' + str(lineNumber) + ' <----------'
		lineNumber = lineNumber + 1
		#grbl_out = s.readline() # Wait for response with carriage return
		#print ' : ' + grbl_out.strip()
		print '-----------------------echo-----------------------------'
		character = s.read(1)
		while (character != '>'):
			#print character
			sys.stdout.write(character)
			character = s.read(1)
		print '---------------------fin echo---------------------------'
		print 'Linea de codigo: ' + str(lineNumber)
		lineNumber = lineNumber + 1
 
# Wait here until printing is finished to close serial port and file.
longstring2 = """\
                                     :X-
                                  :X###
                                ;@####@
                              ;M######X
                            -@########$
                          .$##########@
                         =M############-
                        +##############$
                      .H############$=.
         ,/:         ,M##########M;.
      -+@###;       =##########M;
   =%M#######;     :#########M/
-$M###########;   :########/
 ,;X###########; =#######$.
     ;H#########+######M=
       ,+#############+
          /M########@-
            ;M#####%
              +####:
               ,$M-
"""
print longstring2
raw_input("******************EOF******************\n Oprima ENTER para salir.")
 
# Close file and serial port
f.close()
s.close()