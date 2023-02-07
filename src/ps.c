#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "SDL_timer.h"
#include "SDL_audio.h"

#include "notes.h"
#include "ps.h"

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #define sleep(sec) Sleep(1000*(sec))
#else
    #include <unistd.h> //sleep()
#endif

int RandInt()
{
    static int seed = 0;

    seed *= 13;
    seed ^= 0x55555555;

    return seed;
}

static inline void flanger_put( float v1, float v2 )
{
    float ptr2 = flanger_ptr - flanger_size;

    if( ptr2 < 0 ) ptr2 += ( flanger_size * 2 );

    flanger_buf1[ (int)ptr2 ] += v1 / 2;
    flanger_buf2[ (int)ptr2 ] += v2 / 2;
}

static inline void flanger_get( float* v1, float* v2 )
{
    int ptr2;

    flanger_buf1[ (int)flanger_ptr ] /= 1.1;
    flanger_buf2[ (int)flanger_ptr ] /= 1.1;

    *v1 += flanger_buf1[ (int)flanger_ptr ] / 1.5;
    *v2 += flanger_buf2[ (int)flanger_ptr ] / 1.5;

    flanger_ptr++;

    if( flanger_ptr >= flanger_size * 2 )
		flanger_ptr -= flanger_size * 2;

    flanger_timer += 0.0001;

    if( rec_play ) 
		flanger_size = 580;
    else
		flanger_size = ( ( sin( flanger_timer / 10 ) + 1 ) / 2 ) * ( MAX_FLANGER_SIZE - 30 ) + 30;
}

static inline void echo_put( float v1, float v2 )
{
    int ptr2 = echo_ptr - ECHO_SIZE;

    if( ptr2 < 0 ) ptr2 += ( ECHO_SIZE * 2 );
    if( ptr2 < ECHO_SIZE )
		echo_buf1[ ptr2 ] += v1 / 2;
    else
		echo_buf2[ ptr2 ] += v2 / 2;
    //Slow reverb:
    int a;
    for( a = 0; a < REVERB; a ++ )
    {
		ptr2 = echo_ptr + slow_reverb[ a ];

		if( ptr2 < 0 ) ptr2 += ( ECHO_SIZE * 2 ) * ( 1 - ( ptr2 / (ECHO_SIZE * 2) ) );
		if( ptr2 >= ( ECHO_SIZE * 2 ) ) ptr2 -= ( ECHO_SIZE * 2 ) * ( ptr2 / (ECHO_SIZE * 2) );
		if( a & 1 )
		    echo_buf1[ ptr2 ] += v1 / 6;
		else
		    echo_buf2[ ptr2 ] += v2 / 6;
    }
}

static inline void reverb_put( float v1, float v2 )
{
    int ptr2;
    //Slow reverb:
    int a;
    for( a = 0; a < REVERB; a ++ )
    {
		ptr2 = echo_ptr + slow_reverb[ a ];

		if( ptr2 < 0 ) ptr2 += ( ECHO_SIZE * 2 ) * ( 1 - ( ptr2 / (ECHO_SIZE * 2) ) );
		if( ptr2 >= ( ECHO_SIZE * 2 ) ) ptr2 -= ( ECHO_SIZE * 2 ) * ( ptr2 / (ECHO_SIZE * 2) );
		if( a & 1 )
		    echo_buf1[ ptr2 ] += v1 / 6;
		else
		    echo_buf2[ ptr2 ] += v2 / 6;
    }
}

static inline void echo_get( float* v1, float* v2 )
{
    int ptr2;
    //echo_buf1[ echo_ptr ] /= 2;
    //echo_buf2[ echo_ptr ] /= 2;
    *v1 += echo_buf1[ echo_ptr ];
    *v2 += echo_buf2[ echo_ptr ];

    echo_ptr++;

    if( echo_ptr >= ECHO_SIZE * 2 ) 
    {
		echo_ptr = 0;

		int a;

		int r = ( ( rand() & 255 ) * (ECHO_SIZE/2) ) >> 8;
		r += ECHO_SIZE;

		for( a = 0; a < ( ECHO_SIZE * 2 ); a++ )
		{
		    ptr2 = a - r;

		    if( ptr2 < 0 ) ptr2 += ( ECHO_SIZE * 2 );

		    echo_buf1[ a ] = ( echo_buf1[ a ] + echo_buf1[ ptr2 ] ) / 2;
		    echo_buf2[ a ] = ( echo_buf2[ a ] + echo_buf2[ ptr2 ] ) / 2;
		}
    }
}

int offset = 0;
float bound = 0.05;

void main_callback( float* buf, int len )
{
    int a;
    int s;
    int c;

    uint8_t* cur_line;

    float res1, res2;
    float freq;

    for( a = 0; a < len; a++ ) { buf[ a << 1 ] = 0; buf[ ( a << 1 ) + 1 ] = 0; } //Clear buffer

    //Render:
    int tick_changed = 0;

    for( a = 0; a < len; a++ ) 
    {
		tick_changed = 0;

		if( Timer >= TICK_SIZE )
		{ //Increment tick number:
		    Timer = 0; Tick++;
		}

		if( Timer == 0 ) tick_changed = 1;

		//Synths (channels):
		for( s = 0; s < CHANNEL_NUM; s++ )
		{
		    cur_line = patterns[ CurPattern ] + ( Tick * CHANNEL_NUM );

		    if( cur_line[ s ] == 255 ) 
		    { //End of pattern:
				Tick = 0;
				CurPattern++;

				if( patterns[ CurPattern ] == 0 ) { ExitRequest = 1; return; }

				cur_line = patterns[ CurPattern ] + ( Tick * CHANNEL_NUM );
		    }

		    if( syn[ s ] == 0 ) continue;
		    switch( syn[ s ] )
		    {
			case SYNTH_PAD: 
			    if( tick_changed )
			    {
					if( cur_line[ s ] )
					{
					    effect_timer[ s ] = 1;

					    bass_freq[ s ] = pow( 2, (float)( cur_line[ s ] + offset ) / 12.0F );
					    bass_tdelta[ s ] = bass_freq[ s ] / Srate;
					    bass_timer[ s ] = 0;
					}
					if( cur_line[ s ] == 254 )
					    effect_timer[ s ] = 0;
					if( cur_line[ s ] == 253 )
					{
					    effect_timer[ s ] = 0;
					    bound = 0.1;
					}
			    }
			    if( effect_timer[ s ] )
			    {
					effect_timer[ s ] *= 0.99998;

					bass_delta[ s ] += ( bass_tdelta[ s ] - bass_delta[ s ] ) / 2000;
					bass_timer[ s ] += bass_delta[ s ];

					res1 = sin( bass_timer[ s ] ) * sin( bass_timer[ s ] * (sin( effect_timer[ s ] ) * 0.02) ) + cos( bass_timer[ s ] * 0.99 );
					res2 = cos( bass_timer[ s ] * 1.01 ) + cos( bass_timer[ s ] * 0.99 );

					if( res1 > 1 ) res1 = 1;
					if( res1 < -1 ) res1 = -1;
					if( res2 > 1 ) res2 = 1;
					if( res2 < -1 ) res2 = -1;
			    }
			    else
			    {
					res1 = 0;
					res2 = 0;
			    }

			    echo_put( res1 / 8, res2 / 8 );

			    res1 = 0;
			    res2 = 0;

			    break;

			case SYNTH_POLY:
			    if( tick_changed )
			    {
					if( cur_line[ s ] )
					{
					    effect_timer[ s ] = 1;

					    bass_freq[ s ] = pow( 2, (float)( cur_line[ s ] + offset ) / 12.0F );
					    bass_tdelta[ s ] = bass_freq[ s ] / Srate;
					    bass_timer[ s ] = 0;
					}
			    }
			    if( effect_timer[ s ] )
			    {
					effect_timer[ s ] *= 0.99998;

					bass_delta[ s ] += ( bass_tdelta[ s ] - bass_delta[ s ] ) / 1800;
					bass_timer[ s ] += bass_delta[ s ];

					res1 = sin( bass_timer[ s ] ) * effect_timer[ s ];
					res2 = sin( bass_timer[ s ] * 1.01 ) * effect_timer[ s ];
			    }
			    else
			    {
					res1 = 0;
					res2 = 0;
			    }

			    buf[ ( a << 1 ) ] += res1 / 8;
			    buf[ ( a << 1 ) + 1 ] += res2 / 8;

			    echo_put( res1 / 4, res2 / 4 );

			    res1 = 0;
			    res2 = 0;

			    break;

			case SYNTH_EFFECT:
			    if( tick_changed )
			    {
					if( cur_line[ s ] )
					{
					    effect_timer[ s ] = 1;
					    effect_timer2[ s ] = 0;
					}

					if( cur_line[ s ] == 5 ) { effect_lowfilter[ s ] = 4; rec_play = 1; }
					if( cur_line[ s ] == 4 ) effect_lowfilter[ s ] = 3;
					if( cur_line[ s ] == 3 ) effect_lowfilter[ s ] = 2;
					if( cur_line[ s ] == 2 ) effect_lowfilter[ s ] = 1;
					if( cur_line[ s ] == 1 ) effect_lowfilter[ s ] = 0;
			    }
			    if( effect_timer[ s ] )
			    {

					effect_timer[ s ] *= 0.99996;
					effect_timer2[ s ] += 0.2;

					if( effect_lowfilter[ s ] == 4 )
					{
					    effect_timer[ s ] = 1;
					    /*res1 = rec1[ rec_ptr ] * 1.8;
					    res2 = rec2[ rec_ptr ] * 1.8;
					    rec_ptr--;
					    if( rec_ptr < 0 ) rec_ptr = rec_size - 1;*/
					}
					else
					if( effect_lowfilter[ s ] == 3 )
					{
					    res2 = sin( ( effect_timer2[ s ] * 1 ) * effect_timer[ s ] ) * effect_timer[ s ]; 
					    res1 = sin( ( effect_timer2[ s ] * 1.5 ) / effect_timer[ s ] ) * effect_timer[ s ];
					}
					else
					if( effect_lowfilter[ s ] == 2 )
					{
					    res2 = sin( ( effect_timer2[ s ] / 4 ) / effect_timer[ s ] ) * effect_timer[ s ]; 
					    res1 = sin( ( effect_timer2[ s ] / 3.5 ) / effect_timer[ s ] ) * effect_timer[ s ];
					}
					else
					if( effect_lowfilter[ s ] == 1 )
					{
					    res2 = sin( ( effect_timer2[ s ] / 2 ) / effect_timer[ s ] ) * effect_timer[ s ]; 
					    res1 = sin( ( effect_timer2[ s ] / 2.5 ) / effect_timer[ s ] ) * effect_timer[ s ];
					}
					else
					    res2 = res1 = sin( effect_timer2[ s ] / effect_timer[ s ] ) * effect_timer[ s ];
			    }
			    else
			    {
					res1 = 0;
					res2 = 0;
			    }

			    buf[ ( a << 1 ) ] += ( res1 / 9.0F );
			    buf[ ( a << 1 ) + 1 ] += ( res2 / 9.0F );

			    if( effect_lowfilter[ s ] != 4 )

				echo_put( res1 / 5, res2 / 5 );

			    res1 = 0;
			    res2 = 0;

			    break;

			case SYNTH_ACID_BASS: 
			    if( tick_changed )
			    {
					bass_freq[ s ] = pow( 2, (float)( cur_line[ s ] + offset ) / 12.0F );
					bass_tdelta[ s ] = bass_freq[ s ] / Srate;
					bass_timer[ s ] = 0;

					effect_timer[ s ] = 1;
					effect_timer2[ s ] = 0;
			    }

			    bass_delta[ s ] += ( bass_tdelta[ s ] - bass_delta[ s ] ) / 300;
			    bass_timer[ s ] += bass_delta[ s ];

			    effect_timer2[ s ] += 0.02 * (32 - (float)Tick);
			    effect_timer[ s ] *= 0.9997;

			    if( cur_line[ s ] )
			    {
					res2 = res1 = sin( bass_timer[ s ] ) + ( sin( bass_timer[ s ] * 4.01 ) * 0.9 );

					if( res1 > 0.2 ) res1 = 0.1;
					if( res1 < -0.2 ) res1 = -0.1;
					if( res2 > 0.2 ) res2 = 0.1;
					if( res2 < -0.2 ) res2 = -0.1;

					//res1 += ( sin( bass_timer[ s ] ) * cos( effect_timer2[ s ] * effect_timer[ s ] ) * 0.07 );
					//res2 += ( sin( bass_timer[ s ] ) * cos( effect_timer2[ s ] * effect_timer[ s ] ) * 0.07 );

					res1 *= effect_timer[ s ];
					res2 *= effect_timer[ s ];
			    }
			    else { res1 = 0; res2 = 0; }

			    echo_put( res1, res2 );

			    buf[ ( a << 1 ) ] += res1;
			    buf[ ( a << 1 ) + 1 ] += res2;

			    break;

			case SYNTH_BASS:
			case SYNTH_BASS_TINY:
			    if( tick_changed )
			    {
					bass_freq[ s ] = pow( 2, (float)( cur_line[ s ] + offset ) / 12.0F );
					bass_tdelta[ s ] = bass_freq[ s ] / Srate;
					bass_timer[ s ] = 0;

					effect_timer[ s ] = 1;
			    }

			    bass_delta[ s ] += ( bass_tdelta[ s ] - bass_delta[ s ] ) / 300;
			    bass_timer[ s ] += bass_delta[ s ];

			    effect_timer[ s ] *= 0.9999;

			    if( cur_line[ s ] )
			    {
					res1 = sin( bass_timer[ s ] ) + sin( bass_timer[ s ] * 2.01 );
					res2 = cos( bass_timer[ s ] ) + cos( bass_timer[ s ] * 2.006 );

					if( res1 > bound ) res1 = 0.05;
					if( res1 < -bound ) res1 = -0.05;
					if( res2 > bound ) res2 = 0.05;
					if( res2 < -bound ) res2 = -0.05;
			    }
			    else
			    {
					res1 = 0;
					res2 = 0;
			    }
			    if( bound > 0.1 ) { res1 *=  effect_timer[ s ]; res2 *= effect_timer[ s ]; }
			    if( syn[ s ] == SYNTH_BASS_TINY ) 
			    {
					res1 *= 0.9; res2 *= 0.9;
			    }
			    else
			    {
					if( !rec_play )
					{
					    flanger_put( res1, res2 );
					    res1 /= 1.5; res2 /= 1.5;
					    flanger_get( &res1, &res2 );
					}
			    }
			    echo_put( res1, res2 );

			    buf[ ( a << 1 ) ] += res1;
			    buf[ ( a << 1 ) + 1 ] += res2;

			    break;

			case SYNTH_HAT:
			    if( tick_changed ) 
			    {
					if( cur_line[ s ] )
					    hat_timer[ s ] = 2;
					if( cur_line[ s ] == 254 )
					{
					    offset ++;
					    Fadeout = 1;

					    break;
					}
					if( cur_line[ s ] == 253 )
					{
					    syn[ 3 ] = SYNTH_ACID_BASS;
					    start_recorder = 1;
					}
					if( cur_line[ s ] == 252 )
				    	offset --;
			    }

			    if( cur_line[ s ] == 254 ) break;
			    if( cur_line[ s ] == 253 ) break;
			    if( cur_line[ s ] == 252 ) break;

			    hat_timer[ s ] -= 0.0004;

			    if( hat_timer[ s ] < 0 ) { hat_timer[ s ] = 0; break; }

			    res1 = ( ( (float)(RandInt()&0x7FFF) / 32000.0F ) - 0.5 ) * ( (float)cur_line[ s ] / 10.0F );
			    res2 = ( ( (float)(RandInt()&0x7FFF) / 32000.0F ) - 0.5 ) * ( (float)cur_line[ s ] / 10.0F );

			    if( res1 > hat_old1[ s ] ) hat_old1[ s ] += 0.03;
			    if( res1 < hat_old1[ s ] ) hat_old1[ s ] -= 0.03;
			    if( res2 > hat_old2[ s ] ) hat_old2[ s ] += 0.03;
			    if( res2 < hat_old2[ s ] ) hat_old2[ s ] -= 0.03;

			    res1 = hat_old1[ s ];
			    res2 = hat_old2[ s ];

			    hat_old1[ s ] = res1;
			    hat_old2[ s ] = res2;

			    res1 *= hat_timer[ s ] * ( (float)cur_line[ s ] / 100.0F );
			    res2 *= hat_timer[ s ] * ( (float)cur_line[ s ] / 100.0F );

			    //reverb_put( res1 / 5, res2 / 5 );
			    buf[ ( a << 1 ) ] += res1 / 1.8;
			    buf[ ( a << 1 ) + 1 ] += res2 / 1.8;

			    break;

			case SYNTH_DRUM: 
			    if( tick_changed ) 
			    {
					if( cur_line[ s ] )
					{
				    	drum_timer[ s ] = 1;
				    	drum_timer2[ s ] = 0;
				    	drum_timer3[ s ] = 1;
					}

			    	drum_timer[ s ] -= 0.001;
			    	drum_timer2[ s ] += 0.03;
			    	drum_timer3[ s ] += 0.003;

			    	if( drum_timer[ s ] < 0 ) { drum_timer[ s ] = 0; break; }
			    	if( 0 )
			    	{
						res1 = drum_timer[ s ] * ( (float)cur_line[ s ] / 12.0F ) * sinf( ((float)cur_line[ s ] / 70) * drum_timer2[ s ] / drum_timer3[ s ] );
						res2 = drum_timer[ s ] * ( (float)cur_line[ s ] / 12.0F ) * sinf( drum_timer2[ s ] / drum_timer3[ s ] + 2 );
			    	}
				}
			    else
			    {
					res1 = drum_timer[ s ] * ( (float)cur_line[ s ] / 20.0F ) * sinf( ((float)cur_line[ s ] / 70) * drum_timer2[ s ] / drum_timer3[ s ] );
					res2 = res1;
					
					float d2 = drum_timer[ s ] * ( (float)cur_line[ s ] / 120.0F ) * sinf( drum_timer2[ s ] / drum_timer3[ s ] + 2 );

					if( Tick & 1 )
					{
					    res1 *= 0.3f;
					    res1 += d2;
					}
					else
					{
					    res2 *= 0.3f;
					    res2 += d2;
					}
			    }

			    float clip = 0.3;

			    if( res1 > clip ) res1 = clip;
			    if( res1 < -clip ) res1 = -clip;
			    if( res2 > clip ) res2 = clip;
			    if( res2 < -clip ) res2 = -clip;

			    buf[ ( a << 1 ) ] += res1;
			    buf[ ( a << 1 ) + 1 ] += res2;

			    res1 = 0;
			    res2 = 0;

			    break;
		    }
		}
		//buf[ ( a << 1 ) ] = 0;
		//buf[ ( a << 1 ) + 1 ] = 0;
		echo_get( &res1, &res2 );

		buf[ ( a << 1 ) ] += res1;
		buf[ ( a << 1 ) + 1 ] += res2;

		if( start_recorder )
		{
		    rec1[ rec_ptr ] = buf[ ( a << 1 ) ];
		    rec2[ rec_ptr ] = buf[ ( a << 1 ) + 1 ];

		    // if( RandInt() & 15 ) rec1[ rec_ptr ] += 0.2 * ( (float)( RandInt() & 255 ) / 255 );
		    // if( RandInt() & 15 ) rec2[ rec_ptr ] += 0.2 * ( (float)( RandInt() & 255 ) / 255 );

		    rec_ptr++;

		    if( rec_ptr >= REC_SIZE )
		    {
				start_recorder = 0;
				rec_ptr --;
		    }
		}

		if( rec_play )
		{
		    flanger_put( rec1[ rec_ptr ] / 2, rec2[ rec_ptr ] / 2 );

		    res1 = 0; res2 = 0;

		    flanger_get( &res1, &res2 );

		    buf[ ( a << 1 ) ] += res1;
    		buf[ ( a << 1 ) + 1 ] += res2;

		    rec_ptr--;

		    if( rec_ptr < 0 ) rec_ptr = REC_SIZE - 1;
		}

		Timer++;

		if( Fadeout )
		{
		    FadeoutVol -= 0.000001;

		    if( FadeoutVol <= 0 ) { FadeoutVol = 0; ExitRequest = 1; }

		    buf[ ( a << 1 ) ] *= FadeoutVol;
		    buf[ ( a << 1 ) + 1 ] *= FadeoutVol;
		}
    }
}

//Utrom ekzamen... Ja ne gotov :) ... na na na
//11 june 2005

void render_buf( float* buf, int len ) //buf: LRLRLR..; len - number of frames (one frame = LR (Left+Right channel));
{
    main_callback( buf, len );

    //Simple DC blocker:
    if( Volume != 1 ) 
		for( int a = 0; a < len * 2; a++ ) buf[ a ] *= Volume;

    for( int a = 0; a < len; a++ )
    {
        dc_sl += buf[ a * 2 ]; dc_sl /= 2;
        dc_sr += buf[ a * 2 + 1 ]; dc_sr /= 2;
    }
    for( int a = 0; a < len; a++ )
    {
        //Simple DC blocker:
        float a2 = (float)(len-a) / len;
        float a3 = (float)a / len;

        buf[ a * 2 ] -= ( dc_psl * a2 ) + ( dc_sl * a3 );
        buf[ a * 2 + 1 ] -= ( dc_psr * a2 ) + ( dc_sr * a3 );
    	//Simple volume compression:
        buf[ a * 2 ] /= MaxVolume;
        buf[ a * 2 + 1 ] /= MaxVolume;

        MaxVolume -= 0.0005;

        if( MaxVolume < 1 ) MaxVolume = 1;

        float t1 = buf[ a * 2 ]; if( t1 < 0 ) t1 = -t1;
        float t2 = buf[ a * 2 + 1 ]; if( t2 < 0 ) t2 = -t2;

        if( t1 > MaxVolume ) MaxVolume = t1;
        if( t2 > MaxVolume ) MaxVolume = t2;
    }
    dc_psl = dc_sl; dc_psr = dc_sr;
}

void sdl_audio_callback( void* udata, Uint8* stream, int len )
{
    render_buf( (float*)stream, len / 8 );
}

int sound_init()
{
    for( int a = 0; a < REVERB; a++ ) 
		slow_reverb[ a ] = ( ( rand() & 2047 ) - 1024 ) << 6;

    if( OutMode == 0 )
    {
		SDL_Init(0);

		SDL_AudioSpec a;
		a.freq = Srate;
		a.format = AUDIO_F32;
		a.channels = 2;
		a.samples = Buffsize;
		a.callback = sdl_audio_callback;
		a.userdata = NULL;

		if( SDL_OpenAudio( &a, NULL ) < 0 )
		{
    	    printf( "Couldn't open audio: %s\n", SDL_GetError() );
    	    return -1;
		}

		SDL_PauseAudio( 0 );

		return 0;
    }
    if( OutMode == 1 )
    {
		FILE* f = fopen( export_file_name, "wb" );
    	if( f )
    	{
    	    printf( "Exporting to WAV...\n" );

    	    int fixup1 = 0, fixup2 = 0;
    	    int out_bytes = 0;
    	    int val;

    	    //WAV header:
    	    fwrite( (void*)"RIFF", 1, 4, f );

    	    val = 4 + 24 + 8 + out_bytes;
    	    fixup1 = ftell( f );

    	    fwrite( &val, 4, 1, f );
    	    fwrite( (void*)"WAVE", 1, 4, f );

    	    //WAV FORMAT:
    	    fwrite( (void*)"fmt ", 1, 4, f );

    	    val = 16; fwrite( &val, 4, 1, f );
    	    val = 3; fwrite( &val, 2, 1, f ); //format
    	    val = 2; fwrite( &val, 2, 1, f ); //channels
    	    val = Srate; fwrite( &val, 4, 1, f ); //frames per second
    	    val = Srate * 2 * 4; fwrite( &val, 4, 1, f ); //bytes per second
    	    val = 2 * 4; fwrite( &val, 2, 1, f ); //block align
    	    val = 4 * 8; fwrite( &val, 2, 1, f ); //bits per sample

    	    //WAV DATA:
    	    fwrite( (void*)"data", 1, 4, f );
    	    fixup2 = ftell( f );
    	    fwrite( &out_bytes, 4, 1, f );

    	    while( !ExitRequest )
    	    {
    			float buf[ Buffsize * 2 ];
				render_buf( buf, Buffsize );
    			out_bytes += fwrite( buf, 1, Buffsize * 2 * 4, f );
    	    }

    	    fseek( f, fixup1, SEEK_SET ); val = 4 + 24 + 8 + out_bytes; fwrite( &val, 4, 1, f );
    	    fseek( f, fixup2, SEEK_SET ); val = out_bytes; fwrite( &val, 4, 1, f );

    	    int frames = out_bytes / ( 4 * 2 );

    	    printf( "%d bytes; %d frames; %d seconds\n", out_bytes, frames, frames / Srate );

    	    fclose( f );
    	}
		return 0;
    }
    return -1;
}

void sound_close()
{
    if( OutMode == 0 )
    {
		SDL_CloseAudio();
		SDL_Quit();
    }
}

void int_handler( int param )
{
    ExitRequest = 1;
}

int main( int argc, char* argv[] )
{
    signal( SIGINT, int_handler );
    if( argc == 3 )
    {
		if( strcmp( argv[ 1 ], "-o" ) == 0 )
		{
		    export_file_name = argv[ 2 ];
		    OutMode = 1;
		}
    }

    printf( "\nNightRadio - P.S.\nnightradio@gmail.com\nWarmPlace.ru\n\n" );
    printf( "Usage:\n  just play: ./ps\n  export to WAV: ./ps -o filename.wav\n" );
    printf( "Press CTRL+C to exit\n\n" );

    if( sound_init() ) return 1;

    while( !ExitRequest )
    {
		sleep( 1 );
    }
    sound_close();

    return 0;
}
