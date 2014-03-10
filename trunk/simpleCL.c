/* #######################################################################
    Copyright 2011 Oscar Amoros Huguet, Cristian Garcia Marin

    This file is part of SimpleOpenCL

    SimpleOpenCL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    SimpleOpenCL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SimpleOpenCL. If not, see <http://www.gnu.org/licenses/>.

   #######################################################################

   SimpleOpenCL Version 0.010_27_02_2013

*/

#ifdef __cplusplus
extern "C" {
#endif

#include "simpleCL.h"
#include "simpleCL_error.c"

sclHard* _sclHardList  = NULL;
int _sclHardListLength = 0;

char* _sclLoadProgramSource( const char *filename )
{
	struct stat statbuf;
	FILE *fh;
	char *source;

	fh = fopen( filename, "r" );
	if ( fh == NULL ) {
		fprintf( stderr, "Error on loadProgramSource\n");
		sclPrintErrorFlags( CL_INVALID_PROGRAM );
		return NULL;
	}

	stat( filename, &statbuf );
	source = malloc( statbuf.st_size + 1 );

	if( fread( source, statbuf.st_size, 1, fh ) != 1 ) {
		fprintf( stderr, "Error on loadProgramSource\n");
		sclPrintErrorFlags( CL_INVALID_PROGRAM );
	}

	source[ statbuf.st_size ] = '\0';

	fclose( fh );

	return source;
}

cl_program _sclCreateProgram( char* program_source, cl_context context )
{
	cl_program program;
	cl_int err;

	program = clCreateProgramWithSource( context, 1, (const char**)&program_source, NULL, &err );
	if ( err!=CL_SUCCESS ) {
		fprintf( stderr, "Error on createProgram\n" );
		sclPrintErrorFlags( err );
	}

	return program;
}

void _sclBuildProgram( cl_program program, cl_device_id devices, const char* pName )
{
	cl_int err;
	char build_c[4096];

	err = clBuildProgram( program, 0, NULL, NULL, NULL, NULL );
	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "Error on buildProgram\n" );
		sclPrintErrorFlags( err );
		fprintf( stderr, "RequestingInfo\n" );
		clGetProgramBuildInfo( program, devices, CL_PROGRAM_BUILD_LOG, 4096, build_c, NULL );
		fprintf( stderr, "Build Log for %s_program:\n%s\n", pName, build_c );
	}
}

cl_kernel _sclCreateKernel( sclSoft software ) {
	cl_kernel kernel;
	cl_int err;

	kernel = clCreateKernel( software.program, software.kernelName, &err );
	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "Error on createKernel %s\n", software.kernelName );
		sclPrintErrorFlags( err );
	}

	return kernel;
}

cl_event sclLaunchKernel( sclHard hardware, sclSoft software, size_t *global_work_size, size_t *local_work_size) {
	cl_event myEvent=NULL;
	cl_int err;

	err = clEnqueueNDRangeKernel( hardware.queue, software.kernel, 2, NULL, global_work_size, local_work_size, 0, NULL, &myEvent );
	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "Error on launchKernel %s\n", software.kernelName );
		sclPrintErrorFlags(err);
	}

	sclFinish( hardware );
	return myEvent;
}

cl_event sclEnqueueKernel( sclHard hardware, sclSoft software, size_t *global_work_size, size_t *local_work_size) {
	cl_event myEvent=NULL;
	cl_int err;

	err = clEnqueueNDRangeKernel( hardware.queue, software.kernel, 2, NULL, global_work_size, local_work_size, 0, NULL, &myEvent );
	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "Error on launchKernel %s\n", software.kernelName );
		sclPrintErrorFlags(err);
	}

	return myEvent;
}

void sclReleaseClSoft( sclSoft soft ) {
	clReleaseKernel( soft.kernel );
	clReleaseProgram( soft.program );
}

void sclReleaseClHard( sclHard hardware ){
	clReleaseCommandQueue( hardware.queue );
	clReleaseContext( hardware.context );
}

void sclRetainClHard( sclHard hardware ) {
	clRetainCommandQueue( hardware.queue );
	clRetainContext( hardware.context );
}

void sclReleaseAllHardware ( sclHard* hardList, int found ) {
	int i;

	for ( i = 0; i < found; ++i ) {
		sclReleaseClHard( hardList[i] );
	}
}

void sclRetainAllHardware ( sclHard* hardList, int found ) {
	int i;

	for ( i = 0; i < found; ++i ) {
		sclRetainClHard( hardList[i] );
	}
}

void sclReleaseMemObject( cl_mem object ) {
	cl_int err;

	err = clReleaseMemObject( object );
	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "Error on sclReleaseMemObject\n" );
		sclPrintErrorFlags(err);
	}
}

void sclPrintDeviceNamePlatforms( sclHard* hardList, int found ) {
	int i;
	cl_char deviceName[1024];
	cl_char platformVendor[1024];
	cl_char platformName[1024];

	for ( i = 0; i < found; ++i ) {
		clGetPlatformInfo( hardList[i].platform, CL_PLATFORM_NAME, sizeof(cl_char)*1024, platformName, NULL );
		clGetPlatformInfo( hardList[i].platform, CL_PLATFORM_VENDOR, sizeof(cl_char)*1024, platformVendor, NULL );
		clGetDeviceInfo( hardList[i].device, CL_DEVICE_NAME, sizeof(cl_char)*1024, deviceName, NULL );
		fprintf( stdout, " Device %d\n Platform name: %s\n Vendor: %s\n Device name: %s\n",
				hardList[i].devNum, platformName, platformVendor, deviceName );
	}
}

void sclPrintHardwareStatus( sclHard hardware ) {
	cl_int err;
	char platform[100];
	cl_bool deviceAV;

	err = clGetPlatformInfo( hardware.platform,
			CL_PLATFORM_NAME,
			sizeof(char)*100,
			platform,
			NULL );
	if ( err == CL_SUCCESS ) { fprintf( stdout, "Platform object alive\n" ); }
	else { sclPrintErrorFlags( err ); }

	err = clGetDeviceInfo( hardware.device,
			CL_DEVICE_AVAILABLE,
			sizeof(cl_bool),
			(void*)(&deviceAV),
			NULL );
	if ( err == CL_SUCCESS && deviceAV ) {
		fprintf( stdout, "Device object alive and device available\n" );
	}
	else if ( err == CL_SUCCESS ) {
		fprintf( stdout, "Device object alive but NOT available\n");
	}
	else {
		fprintf( stderr, "Device object not alive\n");
	}
}

void _sclCreateQueues( sclHard* hardList, int found ) {
	int i;
	cl_int err;

	for ( i = 0; i < found; ++i ) {
		hardList[i].queue = clCreateCommandQueue( hardList[i].context, hardList[i].device,
							 CL_QUEUE_PROFILING_ENABLE, &err );
		if ( err != CL_SUCCESS ) {
			fprintf( stderr, "Error creating command queue %d\n", i );
		}
	}
}

void _sclSmartCreateContexts( sclHard* hardList, int found ) {
	cl_device_id deviceList[16];
	cl_context context;
	char var_queries1[1024];
	char var_queries2[1024];
	ptsclHard groups[10][20];
	int i, j, groupSet = 0;
	int groupSizes[10];
	int nGroups = 0;
	cl_int err;

	for ( i = 0; i < found; ++i ) { /* Group generation */
		clGetPlatformInfo( hardList[i].platform, CL_PLATFORM_NAME, 1024, var_queries1, NULL );

		if (  nGroups == 0 ) {
			groups[0][0] = &(hardList[0]);
			nGroups++;
			groupSizes[0] = 1;
		}
		else {
			groupSet=0;
			for ( j = 0; j < nGroups; ++j ) {
				clGetPlatformInfo( groups[j][0]->platform, CL_PLATFORM_NAME, 1024, var_queries2, NULL );
				if ( strcmp( var_queries1, var_queries2 ) == 0 &&
						hardList[i].deviceType == groups[j][0]->deviceType &&
						hardList[i].maxPointerSize == groups[j][0]->maxPointerSize ) {
					groups[j][ groupSizes[j] ] = &(hardList[i]);
					groupSizes[j]++;
					groupSet = 1;
				}
			}
			if ( !groupSet ) {
				groups[nGroups][0] = &(hardList[i]);
				groupSizes[nGroups] = 1;
				nGroups++;
			}
		}
	}

	for ( i = 0; i < nGroups; ++i ) { /* Context generation */
		fprintf( stdout, "Group %d with %d devices\n", i+1, groupSizes[i] );
		for ( j = 0; j < groupSizes[i]; ++j ) {
			deviceList[j] = groups[i][j]->device;
		}
		context = clCreateContext( 0, groupSizes[i], deviceList, NULL, NULL, &err );
		if ( err != CL_SUCCESS ) {
			fprintf( stderr, "Error creating context on device %d\n", i );
		}
		for ( j = 0; j < groupSizes[i]; ++j ) {
			groups[i][j]->context = context;
		}
	}
}

int _sclGetMaxComputeUnits( cl_device_id device ) {
	cl_uint nCompU;

	clGetDeviceInfo( device, CL_DEVICE_MAX_COMPUTE_UNITS, 4, (void *)&nCompU, NULL );

	return (int)nCompU;
}

unsigned long int _sclGetMaxMemAllocSize( cl_device_id device ){
	cl_ulong mem;

	clGetDeviceInfo( device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, 8, (void *)&mem, NULL );

	return (unsigned long int)mem;
}

cl_device_type _sclGetDeviceType( cl_device_id device ) {
	cl_device_type dev_type;

	clGetDeviceInfo(device,CL_DEVICE_TYPE,sizeof(dev_type),&dev_type,NULL);

	return dev_type;
}

sclHard sclGetFastestDevice( sclHard* hardList, int found ) {
	int i, maxCpUnits = 0, device = 0;

	for ( i = 0; i < found ; ++i ) {
		fprintf( stdout, "Device %d Compute Units %d\n", i, hardList[i].nComputeUnits );
		if ( maxCpUnits < hardList[i].nComputeUnits ) {
			device = i;
			maxCpUnits = hardList[i].nComputeUnits;
		}
	}

	return hardList[ device ];
}

sclHard* sclGetAllHardware( int* found ) {
	int i, j;
	cl_uint nPlatforms=0, nDevices=0;

	*found=0;

	cl_platform_id *platforms;
	cl_int err;
	cl_device_id *devices;

	platforms    = malloc( sizeof(cl_platform_id) * 8 );
	devices      = malloc( sizeof(cl_device_id) * 16 );
	_sclHardList = malloc( sizeof(sclHard) * 16 );

	err = clGetPlatformIDs( 8, platforms, &nPlatforms );
	if ( err != CL_SUCCESS) {
		fprintf(stderr,"Error in clGetPlatformIDs\n");
		sclPrintErrorFlags(err);
	}

	if ( nPlatforms == 0 ) {
		fprintf( stderr, "No OpenCL platforms found\n");
		return NULL;
	}
	else {
		for ( i = 0; i < (int)nPlatforms; ++i ) {
			err = clGetDeviceIDs( platforms[i], CL_DEVICE_TYPE_ALL, 16, devices, &nDevices );
			if ( err != CL_SUCCESS ) {
				fprintf( stderr, "Error clGetDeviceIDs\n" );
				sclPrintErrorFlags( err );
			}

			if ( nDevices == 0 ) {
				fprintf( stderr, "No OpenCL enabled device found\n");
			}
			else {
				for ( j = 0; j < (int)nDevices; ++j ) {
					_sclHardList[ *found ].platform       = platforms[ i ];
					_sclHardList[ *found ].device         = devices[ j ];
					_sclHardList[ *found ].nComputeUnits  = _sclGetMaxComputeUnits( _sclHardList[ *found ].device );
					_sclHardList[ *found ].maxPointerSize = _sclGetMaxMemAllocSize( _sclHardList[ *found ].device );
					_sclHardList[ *found ].deviceType     = _sclGetDeviceType( _sclHardList[ *found ].device );
					_sclHardList[ *found ].devNum         = *found;
					(*found)++;
				}
			}
		}
		_sclHardListLength = *found;
		_sclSmartCreateContexts( _sclHardList, *found );
		_sclCreateQueues( _sclHardList, *found );
	}
#ifdef DEBUG
	sclPrintDeviceNamePlatforms( _sclHardList, *found );
#endif
	sclRetainAllHardware( _sclHardList, *found );

	return _sclHardList;
}

sclHard sclGetGPUHardware( int nDevice, int* found ) {
	int i;
	sclHard hardware;
	cl_int err;
	int nDevices=0;
	cl_char vendor_name[1024];
	cl_char device_name[1024];

	*found = 1;

	for ( i = 0; i < _sclHardListLength; ++i ) {
		if ( _sclHardList[i].deviceType == CL_DEVICE_TYPE_GPU ) {
			nDevices++;
			if ( nDevices-1 == nDevice ) {
				hardware = _sclHardList[i];
				break;
			}
		}
	}

	if ( nDevices == 0 ) {
		fprintf( stderr, "No OpenCL enabled GPU found\n");
		*found = 0;

		return hardware;
	}

	if (nDevice >= nDevices) {
		fprintf( stderr, "No OpenCL device GPU found.n");
		*found = 0;

		return hardware;
	}

	vendor_name[0] = '\0';
	device_name[0] = '\0';

	err = (clGetDeviceInfo( hardware.device, CL_DEVICE_VENDOR, sizeof(vendor_name), vendor_name, NULL )
			 ||clGetDeviceInfo( hardware.device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL ));

	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "Error 2\n" );
		sclPrintErrorFlags( err );
	}

	fprintf( stdout, "Using device vendor: %s\nDevice name: %s\n",vendor_name,device_name);

	return hardware;
}

sclHard sclGetCPUHardware( int nDevice, int* found ) {
	int i;
	sclHard hardware;
	cl_int err;
	int nDevices=0;
	cl_char vendor_name[1024];
	cl_char device_name[1024];

	*found = 1;

	for ( i = 0; i < _sclHardListLength; ++i ) {
		if ( _sclHardList[i].deviceType == CL_DEVICE_TYPE_CPU ) {
			nDevices++;
			if ( nDevices-1 == nDevice ) {
				hardware = _sclHardList[i];
				break;
			}
		}
	}

	if ( nDevices == 0 ) {
		fprintf( stderr, "No OpenCL enabled CPU found\n");
		*found = 0;

		return hardware;
	}

	if (nDevice >= nDevices) {
		fprintf( stderr, "No OpenCL device CPU found\n");
		*found = 0;

		return hardware;
	}

	vendor_name[0] = '\0';
	device_name[0] = '\0';

	err = (clGetDeviceInfo( hardware.device, CL_DEVICE_VENDOR, sizeof(vendor_name), vendor_name, NULL )
			 ||clGetDeviceInfo( hardware.device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL ));

	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "Error 2\n" );
		sclPrintErrorFlags( err );
	}

	fprintf( stdout, "Using device vendor: %s\nDevice name: %s\n",vendor_name,device_name);

	return hardware;
}

sclSoft sclGetCLSoftware( char* path, char* name, sclHard hardware ) {
	sclSoft software;
	/* Load program source
	 ########################################################### */
	char *source = _sclLoadProgramSource( path );
	/* ########################################################### */

	sprintf( software.kernelName, "%s", name);

	/* Create program objects from source
	 ########################################################### */
	software.program = _sclCreateProgram( source, hardware.context );
	/* ########################################################### */

	/* Build the program (compile it)
	############################################ */
	_sclBuildProgram( software.program, hardware.device, name );
	/* ############################################ */

	/* Create the kernel object
	 ########################################################################## */
	software.kernel = _sclCreateKernel( software );
	/* ########################################################################## */

	return software;
}

cl_mem sclMalloc( sclHard hardware, cl_int mode, size_t size ) {
	cl_mem buffer;
	cl_int err;

	buffer = clCreateBuffer( hardware.context, mode, size, NULL, &err );
	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "clMalloc Error\n" );
		sclPrintErrorFlags( err );
	}

	return buffer;
}

cl_mem sclMallocWrite( sclHard hardware, cl_int mode, size_t size, void* hostPointer ){
	cl_mem buffer;

	buffer = sclMalloc( hardware, mode, size );

	cl_int err;

	if ( buffer == NULL ) {
		fprintf( stderr, "clMallocWrite Error on clCreateBuffer\n" );
	}
	err = clEnqueueWriteBuffer( hardware.queue, buffer, CL_TRUE, 0, size, hostPointer, 0, NULL, NULL );
	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "clMallocWrite Error on clEnqueueWriteBuffer\n" );
		sclPrintErrorFlags( err );
	}

	return buffer;
}

void sclWrite( sclHard hardware, size_t size, cl_mem buffer, void* hostPointer ) {
	cl_int err;

	err = clEnqueueWriteBuffer( hardware.queue, buffer, CL_TRUE, 0, size, hostPointer, 0, NULL, NULL );
	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "clWrite Error\n" );
		sclPrintErrorFlags( err );
	}
}

void sclRead( sclHard hardware, size_t size, cl_mem buffer, void *hostPointer ) {
	cl_int err;

	err = clEnqueueReadBuffer( hardware.queue, buffer, CL_TRUE, 0, size, hostPointer, 0, NULL, NULL );
	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "clRead Error\n" );
		sclPrintErrorFlags( err );
	}
}

cl_int sclFinish( sclHard hardware ){
	cl_int err;

	err = clFinish( hardware.queue );
	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "Error clFinish\n" );
		sclPrintErrorFlags( err );
	}

	return err;
}

cl_ulong sclGetEventTime( sclHard hardware, cl_event event ){
	cl_ulong elapsedTime, startTime, endTime;

	sclFinish( hardware );

	clGetEventProfilingInfo( event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startTime, NULL);
	clGetEventProfilingInfo( event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endTime, NULL);

	elapsedTime = endTime-startTime;

	return elapsedTime;
}

void sclSetKernelArg( sclSoft software, int argnum, size_t typeSize, void *argument ){
	cl_int err;

	err = clSetKernelArg( software.kernel, argnum, typeSize, argument );
	if ( err != CL_SUCCESS ) {
		fprintf( stderr, "Error clSetKernelArg number %d\n", argnum );
		sclPrintErrorFlags( err );
	}
}

void _sclWriteArgOnAFile( int argnum, void* arg, size_t size, const char* diff ) {
	FILE *out;
	char filename[150];

	sprintf( filename, "../data/arg%d%s", argnum, diff );

	out = fopen( filename, "w+");

	fwrite( arg, 1, size, out );

	fclose(out);
}

 void _sclVSetKernelArgs( sclSoft software, const char *sizesValues, va_list argList ) {
	const char *p;
	int argCount = 0;
	void* argument;
	size_t actual_size;

	for( p = sizesValues; *p != '\0'; p++ ) {
		if ( *p == '%' ) {
			switch( *++p ) {
				case 'a':
					actual_size = va_arg( argList, size_t );
					argument = va_arg( argList, void* );
					sclSetKernelArg( software, argCount, actual_size, argument );
					argCount++;
					break;
				case 'v':
					argument = va_arg( argList, void* );
					sclSetKernelArg( software, argCount, sizeof(cl_mem) , argument );
					argCount++;
					break;
				case 'N':
					actual_size = va_arg( argList, size_t );
					sclSetKernelArg( software, argCount, actual_size, NULL );
					argCount++;
					break;
				default:
					break;
			}
		}
	}
}

void sclSetKernelArgs( sclSoft software, const char *sizesValues, ... ){
	va_list argList;

	va_start( argList, sizesValues );

	_sclVSetKernelArgs( software, sizesValues, argList );

	va_end( argList );
}

cl_event sclSetArgsLaunchKernel( sclHard hardware, sclSoft software, size_t *global_work_size, size_t *local_work_size,
				const char *sizesValues, ... ) {
	va_list argList;
	cl_event event;

	va_start( argList, sizesValues );

	_sclVSetKernelArgs( software, sizesValues, argList );

	va_end( argList );

	event = sclLaunchKernel( hardware, software, global_work_size, local_work_size );

	return event;
}

cl_event sclSetArgsEnqueueKernel( sclHard hardware, sclSoft software, size_t *global_work_size, size_t *local_work_size,
				 const char *sizesValues, ... ) {
	va_list argList;
	cl_event event;

	va_start( argList, sizesValues );

	_sclVSetKernelArgs( software, sizesValues, argList );

	va_end( argList );

	event = sclEnqueueKernel( hardware, software, global_work_size, local_work_size );

	return event;
}

cl_event sclManageArgsLaunchKernel( sclHard hardware, sclSoft software, size_t *global_work_size, size_t *local_work_size,
					const char* sizesValues, ... ) {
	va_list argList;
	cl_event event;
	const char *p;
	int argCount = 0, outArgCount = 0, inArgCount = 0, i;
	void* argument;
	size_t actual_size;
	cl_mem outBuffs[30];
	cl_mem inBuffs[30];
	size_t sizesOut[30];
	typedef unsigned char* puchar;
	puchar outArgs[30];

	va_start( argList, sizesValues );

	for( p = sizesValues; *p != '\0'; p++ ) {
		if ( *p == '%' ) {
			switch( *++p ) {
				case 'a': /* Single value non pointer argument */
					actual_size = va_arg( argList, size_t );
					argument = va_arg( argList, void* );
					sclSetKernelArg( software, argCount, actual_size, argument );
					argCount++;
					break;
				case 'v': /* Buffer or image object void* argument */
					argument = va_arg( argList, void* );
					sclSetKernelArg( software, argCount, sizeof(cl_mem) , argument );
					argCount++;
					break;
				case 'N': /* Local memory object using NULL argument */
					actual_size = va_arg( argList, size_t );
					sclSetKernelArg( software, argCount, actual_size, NULL );
					argCount++;
					break;
				case 'w': /* */
					sizesOut[ outArgCount ] = va_arg( argList, size_t );
					outArgs[ outArgCount ] = (unsigned char*)va_arg( argList, void* );
					outBuffs[ outArgCount ] = sclMalloc( hardware, CL_MEM_WRITE_ONLY,
											sizesOut[ outArgCount ] );
					sclSetKernelArg( software, argCount, sizeof(cl_mem), &outBuffs[ outArgCount ] );
					argCount++;
					outArgCount++;
					break;
				case 'r': /* */
					actual_size = va_arg( argList, size_t );
					argument = va_arg( argList, void* );
					inBuffs[ inArgCount ] = sclMallocWrite( hardware, CL_MEM_READ_ONLY, actual_size,
											argument );
					sclSetKernelArg( software, argCount, sizeof(cl_mem), &inBuffs[ inArgCount ] );
					inArgCount++;
					argCount++;
					break;
				case 'R': /* */
					sizesOut[ outArgCount ] = va_arg( argList, size_t );
					outArgs[ outArgCount ] = (unsigned char*)va_arg( argList, void* );
					outBuffs[ outArgCount ] = sclMallocWrite( hardware, CL_MEM_READ_WRITE,
											sizesOut[ outArgCount ],
											outArgs[ outArgCount ] );
					sclSetKernelArg( software, argCount, sizeof(cl_mem), &outBuffs[ outArgCount ] );
					argCount++;
					outArgCount++;
					break;
				case 'g':
					actual_size = va_arg( argList, size_t );
					inBuffs[ inArgCount ] = sclMalloc( hardware, CL_MEM_READ_WRITE, actual_size );
					sclSetKernelArg( software, argCount, sizeof(cl_mem), &inBuffs[ inArgCount ] );
					inArgCount++;
					argCount++;
					break;
				default:
					break;
			}
		}
	}

	va_end( argList );

	event = sclLaunchKernel( hardware, software, global_work_size, local_work_size );

	for ( i = 0; i < outArgCount; i++ ) {
		sclRead( hardware, sizesOut[i], outBuffs[i], outArgs[i] );
	}

	sclFinish( hardware );

	for ( i = 0; i < outArgCount; i++ ) {
		sclReleaseMemObject( outBuffs[i] );
	}

	for ( i = 0; i < inArgCount; i++ ) {
		sclReleaseMemObject( inBuffs[i] );
	}

	return event;
}

#ifdef __cplusplus
}
#endif
