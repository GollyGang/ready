[![Build Status](https://travis-ci.org/GollyGang/ready.svg?branch=gh-pages)](https://travis-ci.org/GollyGang/ready)

## Ready ##

Ready is a program for exploring [continuous](http://www.wolframscience.com/nksonline/section-4.8) and discrete cellular automata, including [reaction-diffusion](http://mrob.com/pub/comp/xmorphia/) systems, on grids and arbitrary meshes. [OpenCL](http://en.wikipedia.org/wiki/OpenCL) is used as the computation engine, to take advantage of the [many-core architectures](http://herbsutter.com/welcome-to-the-jungle/) on graphics cards and modern CPUs. OpenCL also allows rules to be written in a text format and compiled on the fly. Ready supports a compact [XML](http://en.wikipedia.org/wiki/XML)-based file format so that images and rules can be shared easily.

Ready supports 1D, 2D and 3D data, as well as polygonal and polyhedral meshes.

Download Ready 0.8 here: 
  * [Windows 64 bit](https://github.com/GollyGang/ready/releases/download/0.8/Ready-0.8-Windows-64bit.zip) ([32 bit](https://github.com/GollyGang/ready/releases/download/0.8/Ready-0.8-Windows-32bit.zip))
  * [MacOS (10.6+)](https://github.com/GollyGang/ready/releases/download/0.8/Ready-0.8-Mac.dmg)
  * Linux: build instructions in [BUILD.txt](https://github.com/GollyGang/ready/blob/gh-pages/BUILD.txt#L102) 

More details on the [releases tab](https://github.com/GollyGang/ready/releases).

[Changes](https://GollyGang.github.io/ready/Help/changes.html), [To-do list](https://GollyGang.github.io/ready/TODO.txt), [Credits](https://GollyGang.github.io/ready/Help/credits.html)

For questions, [join our mailing list](https://groups.google.com/forum/#!forum/reaction-diffusion). Or you can email [tim.hutton@gmail.com](mailto:tim.hutton@gmail.com).

Screenshots:

![![](https://lh4.googleusercontent.com/-_M9NwfsZOEU/Uo3nISJFpXI/AAAAAAAAIQE/8dt59x8IDvc/s144/yang2006.png)](https://lh4.googleusercontent.com/-_M9NwfsZOEU/Uo3nISJFpXI/AAAAAAAAIQE/8dt59x8IDvc/s144/yang2006.png)
![![](https://lh3.googleusercontent.com/-tbp9Y42reJg/Uo3nFECRvkI/AAAAAAAAIPI/0vv38WYrLSU/s144/mccabe.png)](https://lh3.googleusercontent.com/-tbp9Y42reJg/Uo3nFECRvkI/AAAAAAAAIPI/0vv38WYrLSU/s144/mccabe.png)
![![](https://lh5.googleusercontent.com/-jvk-BzbFNlU/Uo3nB1Qsl8I/AAAAAAAAIO0/GTilEpY19GY/s144/brusselator.png)](https://lh5.googleusercontent.com/-jvk-BzbFNlU/Uo3nB1Qsl8I/AAAAAAAAIO0/GTilEpY19GY/s144/brusselator.png)
![![](https://lh5.googleusercontent.com/-VwhGWIHCfxw/Uo3nH-SAaxI/AAAAAAAAIP8/mpzSvTXUxxw/s144/wills_orbits.png)](https://lh5.googleusercontent.com/-VwhGWIHCfxw/Uo3nH-SAaxI/AAAAAAAAIP8/mpzSvTXUxxw/s144/wills_orbits.png)
[![](https://lh6.googleusercontent.com/-IR42YCbSsqw/UCKFmoEFkpI/AAAAAAAAF8A/UblNhiOtHUE/s144/s1.png)](https://picasaweb.google.com/110214848059767137292/Ready04Screenshots#5774324570878481042)
[![](https://lh3.googleusercontent.com/-1xtb7dTldiI/UCKFmpVe0kI/AAAAAAAAF8A/ZF4En9R1fDo/s144/s10.png)](https://picasaweb.google.com/110214848059767137292/Ready04Screenshots#5774324571219874370)
[![](https://lh3.googleusercontent.com/-ABZodETVJQU/UCKFm1SN6qI/AAAAAAAAF8A/Ci97Nk3NCtE/s144/s2.png)](https://picasaweb.google.com/110214848059767137292/Ready04Screenshots#5774324574427409058) [![](https://lh6.googleusercontent.com/-3dUT0moQmH4/UCKFnQn-yAI/AAAAAAAAF8A/Lvchlc1rM0g/s144/s3.png)](https://picasaweb.google.com/110214848059767137292/Ready04Screenshots#5774324581766449154) [![](https://lh4.googleusercontent.com/-lehLT1C23bA/UCKFp816pRI/AAAAAAAAF8A/8rBqL9faxLs/s144/s4.png)](https://picasaweb.google.com/110214848059767137292/Ready04Screenshots#5774324627995796754) [![](https://lh4.googleusercontent.com/-AYzUA8X_bvg/UCKFoJW95GI/AAAAAAAAF8A/M8XwkJd7Bas/s144/s5.png)](https://picasaweb.google.com/110214848059767137292/Ready04Screenshots#5774324596995908706) [![](https://lh3.googleusercontent.com/-44R8OEzf5tM/UCKFqosqQgI/AAAAAAAAF8A/m66wxKJEU1Y/s144/s6.png)](https://picasaweb.google.com/110214848059767137292/Ready04Screenshots#5774324639768134146) [![](https://lh4.googleusercontent.com/-sEO5LXvey7A/UCKFot3bczI/AAAAAAAAF8A/hL-YSctyIVw/s144/s7.png)](https://picasaweb.google.com/110214848059767137292/Ready04Screenshots#5774324606795739954) [![](https://lh6.googleusercontent.com/-_bn7sCqhWsk/UCKFpOJLHrI/AAAAAAAAF8A/TlwMnccOfGI/s144/s8.png)](https://picasaweb.google.com/110214848059767137292/Ready04Screenshots#5774324615460101810) [![](https://lh5.googleusercontent.com/-s5MB5t_28VM/UCKFtbXdluI/AAAAAAAAF8A/JXuQDNxCyhQ/s144/s9.png)](https://picasaweb.google.com/110214848059767137292/Ready04Screenshots#5774324687729170146)

Press coverage:
  * New Scientist, August 2012: "[First gliders navigate ever-changing Penrose universe](http://www.newscientist.com/article/dn22134-first-gliders-navigate-everchanging-penrose-universe.html)"

Citations:

  * Kawamata I., Yoshizawa S., Takabatake F., Sugawara K., Murata S. ["Discrete DNA Reaction-Diffusion Model for Implementing Simple Cellular Automaton"](https://link.springer.com/chapter/10.1007/978-3-319-41312-9_14) In: Amos M., CONDON A. (eds) Unconventional Computation and Natural Computation. UCNC 2016. Lecture Notes in Computer Science, vol 9726. Springer, Cham.
  * John G. Maisey and John S. S. Denton. ["Dermal Denticle Patterning in the Cretaceous Hybodont Shark Tribodus limae (Euselachii, Hybodontiformes), and Its Implications for the Evolution of Patterning in the Chondrichthyan Dermal Skeleton"](http://www.bioone.org/doi/full/10.1080/02724634.2016.1179200) Journal of Vertebrate Paleontology 36(5):e1179200, 2016 doi: http://dx.doi.org/10.1080/02724634.2016.1179200
  * Elmeligy Abdelhamid SH, Kuhlman CJ, Marathe MV, Mortveit HS, Ravi SS ["GDSCalc: A Web-Based Application for Evaluating Discrete Graph Dynamical Systems"](https://doi.org/10.1371/journal.pone.0133660) PLoS ONE 10(8):e0133660, 2015. https://doi.org/10.1371/journal.pone.0133660
  * Nathanael Aubert and Olaf Witkowski. ["Reaction-Diffusion Risk: Chemical Signaling in a Conquest Game"](https://www.cs.york.ac.uk/nature/ecal2015/late-breaking/164.pdf) Late Breaking Proceedings of the European Conference on Artificial Life 2015, pp. 29-30, 2015.
  * Kaier Wang, Moira L. Steyn-Ross, D. Alistair Steyn-Ross, Marcus T. Wilson, Jamie W. Sleigh, Yoichi Shiraishi. ["Simulations of pattern dynamics for reaction-diffusion systems via SIMULINK"](http://www.biomedcentral.com/1752-0509/8/45) BMC Systems Biology 8(14), 2014.
  * Aubert, Nathanaël, Clément Mosca, Teruo Fujii, Masami Hagiya, and Yannick Rondelez. ["Computer-assisted design for scaling up systems based on DNA reaction networks"](http://www.yannick-rondelez.com/wp-content/uploads/2014/03/Untitled-37758-1.pdf) Journal of The Royal Society Interface 11, no. 93, 2014.
  * Millán, Emmanuel N., Paula Martínez, Graciela Verónica Gil Costa, María Fabiana Piccoli, Alicia Marcela Printista, Carlos Bederian, Carlos García Garino, and Eduardo M. Bringa. ["Parallel implementation of a cellular automata in a hybrid CPU/GPU environment"](http://sedici.unlp.edu.ar/bitstream/handle/10915/31730/Documento_completo.pdf?sequence=1) In XVIII Congreso Argentino de Ciencias de la Computación. 2013.
  * Adam P. Goucher. ["Gliders in cellular automata on Penrose tilings"](http://cp4space.files.wordpress.com/2012/11/2012-penrose-gliders.pdf), Journal of Cellular Automata, Volume 7, Number 5-6, p. 385-392, 2012.

Please cite Ready as:

    Tim Hutton, Robert Munafo, Andrew Trevorrow, Tom Rokicki, Dan Wills. 
    "Ready, a cross-platform implementation of various reaction-diffusion systems." 
    https://github.com/GollyGang/ready

Blog coverage:
  * The Christev Creative, April 2015: "[Negative Worms in Reaction Diffusion](https://christevcreative.com/2015/04/17/negative-worms-in-reaction-diffusion/)"
  * Tim's blog, December 2012: "[Ready 0.5](https://timhutton.github.io/2012/12/20/36454.html)"
  * GPU Science, September 2012: "[Reaction-Diffusion by the Gray-Scott Model with OpenCL](http://gpuscience.com/physicalscience/reaction-diffusion-by-the-gray-scott-model-with-opencl/)"
  * Complex Projective 4-Space, August 2012: "[Are you Ready?](https://cp4space.wordpress.com/2012/08/24/are-you-ready/)"
  * Mikael Hvidtfeldt Christensen, August 2012: "[Reaction-Diffusion Systems](http://blog.hvidtfeldts.net/index.php/2012/08/reaction-diffusion-systems/)"
  * Hacker News, July 2012: "[First glider discovered in a cellular automata on an aperiodic tiling](http://news.ycombinator.com/item?id=4298515)"
  * Aperiodical.com, July 2012: "[Ready: reaction-diffusion simulator](http://aperiodical.com/2012/07/ready-reaction-diffusion-simulator/)" [[2](http://aperiodical.com/2012/08/a-glider-on-an-aperiodic-cellular-automaton-exists/)]
  * Tim's blog, September 2010: "[reaction-diffusion](http://ferkeltongs.livejournal.com/32025.html)"

Other packages:
  * [TexRD](http://www.texrd.com/) (closed source)
  * [Visions of Chaos](http://softology.com.au/voc.htm) (closed source)
  * [CAPOW](http://www.cs.sjsu.edu/~rucker/capow/)

Wanted rules: (help needed)
  * K. Maginu, "Reaction-diffusion equation describing morphogenesis I. waveform stability of stationary wave solutions in a one dimensional model", Math. Biosci. 27:1/2 (1975), 17–98 http://dx.doi.org/10.1016/0025-5564(75)90026-7
  * L.Decker 2003, derived from Maginu's (from TexRD)
  * L.Decker 2002, derived from Ginzburg-Landau (from TexRD)
  * L.Decker 1998, derived from Brusselator (from TexRD)

Videos:

<a href='http://www.youtube.com/watch?feature=player_embedded&v=KJe9H6qS82I' target='_blank'><img src='http://img.youtube.com/vi/KJe9H6qS82I/0.jpg' width='425' height=344 /></a> <a href='http://www.youtube.com/watch?feature=player_embedded&v=5TubQw4f_RU' target='_blank'><img src='http://img.youtube.com/vi/5TubQw4f_RU/0.jpg' width='425' height=344 /></a> <a href='http://www.youtube.com/watch?feature=player_embedded&v=3oqap32-Tg0' target='_blank'><img src='http://img.youtube.com/vi/3oqap32-Tg0/0.jpg' width='425' height=344 /></a> <a href='http://www.youtube.com/watch?feature=player_embedded&v=c9EoI9tw6NE' target='_blank'><img src='http://img.youtube.com/vi/c9EoI9tw6NE/0.jpg' width='425' height=344 /></a> <a href='http://www.youtube.com/watch?feature=player_embedded&v=tZHOGFA1KZE' target='_blank'><img src='http://img.youtube.com/vi/tZHOGFA1KZE/0.jpg' width='425' height=344 /></a> <a href='http://www.youtube.com/watch?feature=player_embedded&v=XYyX4GpzhmQ' target='_blank'><img src='http://img.youtube.com/vi/XYyX4GpzhmQ/0.jpg' width='425' height=344 /></a>
