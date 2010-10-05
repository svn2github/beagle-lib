::
:: windows script to create cuda files for each state count
:: from the generic state count file
:: @author Aaron Darling
::
::
cd ..\..\..\libhmsbeagle\GPU\kernels

FOR %%G IN (4 16 32 48 64 80 128 192) DO (

echo // DO NOT EDIT -- autogenerated file -- edit kernelsX.cu instead > kernels%%G.cu
echo #define STATE_COUNT %%G >> kernels%%G.cu
type kernelsX.cu >> kernels%%G.cu

) 


FOR %%G IN (4 16 32 48 64 80 128 192) DO (

echo // DO NOT EDIT -- autogenerated file -- edit kernelsX.cu instead > kernels_dp_%%G.cu
echo #define STATE_COUNT %%G >> kernels_dp_%%G.cu
echo #define DOUBLE_PRECISION >> kernels_dp_%%G.cu
type kernelsX.cu >> kernels_dp_%%G.cu

) 

