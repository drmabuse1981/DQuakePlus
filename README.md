==========
DQuakePlus
==========
Fork of DQuake>>http://sourceforge.net/projects/pdquake/ 
Original DQuake by Crow_Bar. 
Used parts from:  
QRack by R00K,   
JoeQuake by Joseph,    
FitzQuake by John Fitzburg,  
Kurok by mdave,    
FuhQuake by fuh a norai,   
FteQuake by Spike,   
and DarkPlaces by Lord Havok!   

Features:
- Q1BSP,HLBSP
- MDL, MD2, MD3
- Halflife / CS1.6 Modelformat(without bone controll)
- QMB and Quake 3 Particles
- External Textures (bmp,tga,png,pcx)
- spr,spr32 for sprites
- decals
- skybox from kurok
- color lightmaps (from kurok / and for hlbsp);
- fog
- usermaps browser    
- nodrawtoclient, drawonlytoclient    
- .ent file support    
- ricochets   

==========
How to compile
==========
1)download cygwin>http://quake-1.com/cache/cygwin.rar

2)Extract folder on C://

3)Go in to cygwin folder and launch .bat file

4)Type "cd" and path to psp folder with make files,like a "cd dquakeplus/psp"

5)To compile SLIM EBOOT type "make -f  makeslim install"

  To compile PHAT EBOOT type "make -f makephat install"

==========
Transparency
==========
use prefix "{" for transparent textures like in HalfLife and like Kurok(see kurok.wad for example)     
R:159 G:091 B:083 for transparent textures like a Kurok      
R:0 G:0 B:255 for transparent textures like a Half Life      
little example:      
{mytexture     
For transparent parts on models you need add your models in engine(see DQ+ note in video_hardware_main in psp folder)   

==========
Ricochets
==========
Must be here    
//normal rics    
weapons/rics/ric1.wav   
weapons/rics/ric2.wav   
weapons/rics/ric3.wav   
// wood rics   
weapons/rics/wood_ric1.wav   
weapons/rics/wood_ric2.wav   
weapons/rics/wood_ric3.wav   
// metall rics  
weapons/rics/metal_ric1.wav   
weapons/rics/metal_ric2.wav   
weapons/rics/metal_ric3.wav   
