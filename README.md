Toymodels
------------
A personal repository for physics simulation.

```
src/
	app/
		helloworld/
		PrecomputedAtmosphericScattering/
		PureLatticeGaugeModel/
	lib/
		etc/
		gpu/
		math/
		window/
	test/
		..
	thirdparty/
		imgui/
shader/
	..
```

How to generate the build projects
------------
#### Windows, Visual Studio 2017
	mkdir _build
	cd _build
	cmake .. -G "Visual Studio 15 2017"

<!--
References
------------
#### Precomputed Atmospheric Scattering
- Eric Bruneton and Fabrice Neyret, "Precomputed atmospheric scattering", Proc. of EGSR'08, Vol 27, no 4, pp. 1079--1086 (2008)
- Oskar Elek, "Rendering Parametrizable Planetary Atmosphere with Multiple Scattering in Real-Time", CESCG 2009.
- Egor Yusov, "Outdoor Light Scattering Sample Update", https://software.intel.com/en-us/blogs/2013/09/19/otdoor-light-scattering-sample-update,(2013)
- Sebastien Hillaire, "Physically Based Sky, Atmosphere and Cloud Rendering in Frostbite", SIGGRAPH 2016.
- Giliam de Carpentier and Kohei Ishiyama, "Decima Engine: Advances in Lighting and AA", SIGGRAPH 2017

#### Misc
- iq, "Raymarching - Primitives", https://www.shadertoy.com/view/Xds3zN
- Dave_Hoskins, "Hash without Sine", https://www.shadertoy.com/view/4djSRW
-->
