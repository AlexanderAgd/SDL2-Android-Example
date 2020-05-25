/*
 * "Parallax Scrolling IV - Overdraw Elimination +"
 *
 *   Nghia             <nho@optushome.com.au>
 *   Randi J. Relander <rjrelander@users.sourceforge.net>
 *   David Olofson     <david@olofson.net>
 *
 * This software is released under the terms of the GPL.
 *
 * Contact authors for permission if you want to use this
 * software, or work derived from it, under other terms.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <SDL.h>
#include <SDL_image.h>
#include "images.h"
#include "parallax4.h"


/*----------------------------------------------------------
	...some globals...
----------------------------------------------------------*/

/*
 * Foreground map.
 */
map_data_t foreground_map = {
/*	 0123456789ABCDEF */
	"3333333333333333",
	"3   2   3      3",
	"3   222 3  222 3",
	"3333 22     22 3",
	"3       222    3",
	"3   222 2 2  333",
	"3   2 2 222    3",
	"3   222      223",
	"3        333   3",
	"3  22 23 323  23",
	"3  22 32 333  23",
	"3            333",
	"3 3  22 33     3",
	"3    222  2  3 3",
	"3  3     3   3 3",
	"3333333333333333"
};

map_data_t single_map = {
/*	 0123456789ABCDEF */
	"3333333333333333",
	"3000200030000003",
	"3000222030022203",
	"3333022000002203",
	"3000000022200003",
	"3000222020200333",
	"3000202022200003",
	"3000222000000223",
	"3000000003330003",
	"3002202303230023",
	"3002203203330023",
	"3000000000000333",
	"3030022033000003",
	"3000022200200303",
	"3003000003000303",
	"3333333333333333"
};

/*
 * Middle level map; where the planets are.
 */
map_data_t middle_map = {
/*	 0123456789ABCDEF */
	"   1    1       ",
	"           1   1",
	"  1             ",
	"     1  1    1  ",
	"   1            ",
	"         1      ",
	" 1            1 ",
	"    1   1       ",
	"          1     ",
	"   1            ",
	"        1    1  ",
	" 1          1   ",
	"     1          ",
	"        1       ",
	"  1        1    ",
	"                "
};

/*
 * Background map.
 */
map_data_t background_map = {
/*	 0123456789ABCDEF */
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000",
	"0000000000000000"
};

int detect_runs = 1;


/*----------------------------------------------------------
	...and some code. :-)
----------------------------------------------------------*/

#define	TM_EMPTY	0
#define	TM_KEYED	1
#define	TM_OPAQUE	2

/* Checks if tile is opaqe, empty or color keyed */
int tile_mode(char tile)
{
	switch(tile)
	{
	  case '0':
	  	return TM_OPAQUE;
	  case '1':
	  	return TM_KEYED;
	  case '2':
	  case '3':
	  	return TM_OPAQUE;
	  case '4':
	  	return TM_KEYED;
	}
	return TM_EMPTY;
}

int draw_tile(SDL_Surface *screen, SDL_Surface *tiles, int x, int y, char tile)
{
	SDL_Rect source_rect, dest_rect;

	/* Study the following expression. Typo trap! :-) */
	if(' ' == tile)
		return 0;

	source_rect.x = 0;	/* Only one column, so we never change this. */
	source_rect.y = (tile - '0') * TILE_H;	/* Select tile from image! */
	source_rect.w = TILE_W;
	source_rect.h = TILE_H;

	dest_rect.x = x;
	dest_rect.y = y;

	SDL_BlitSurface(tiles, &source_rect, screen, &dest_rect);

	/* Return area rendered for statistics */
	return dest_rect.w * dest_rect.h;
}

int main(int argc, char* argv[])
{
	SDL_Window *window;
	SDL_Surface	*screen;
	SDL_Surface	*tiles_bmp;
	SDL_Surface	*tiles, *otiles;
	SDL_Rect	border;
	layer_t		*layers;
	SDL_Event	event;
	int		bpp = 0,
			flags = 0,
			verbose = 0,
			use_planets = 1,
			num_of_layers = 7,
			bounce_around = 0,
			wrap = 0,
			alpha = 0;
	int		total_blits,
			total_recursions,
			total_pixels;
	int		peak_blits = 0,
			peak_recursions = 0,
			peak_pixels = 0;
	int		i;
	long		tick1,
			tick2;
	float		dt;

	SDL_Init(SDL_INIT_VIDEO);

	atexit(SDL_Quit);

	for(i = 1; i < argc; ++i)
	{
		if(strncmp(argv[i], "-v", 2) == 0)
			verbose = argv[i][2] - '0';
		else if(strncmp(argv[i], "-a", 2) == 0)
			alpha = atoi(&argv[i][2]);
		else if(strncmp(argv[i], "-b", 2) == 0)
			bounce_around = 1;
		else if(strncmp(argv[i], "-w", 2) == 0)
			wrap = 1;
		else if(strncmp(argv[i], "-l", 2) == 0)
			num_of_layers = atoi(&argv[i][2]);
		else if(strncmp(argv[i], "-np", 3) == 0)
			use_planets = 0;
		//else if(strncmp(argv[i], "-d", 2) == 0)
			//flags |= SDL_DOUBLEBUF;
		else if(strncmp(argv[i], "-f", 2) == 0)
			flags |= SDL_WINDOW_FULLSCREEN;
		else if(strncmp(argv[i], "-nr", 3) == 0)
			detect_runs = 0;
		else
			bpp = atoi(&argv[i][1]);
	}

	layers = calloc(sizeof(layer_t), num_of_layers);
	if (!layers)
	{
		fprintf(stderr, "Failed to allocate layers!\n");
		exit(-1);
	}

	if ( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ))
		printf( "Warning: Linear texture filtering not enabled!" );

    flags |= SDL_WINDOW_SHOWN;
    flags |= SDL_WINDOW_RESIZABLE;

	window = SDL_CreateWindow("DeMo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                        SCREEN_W, SCREEN_H, flags);
	if (!window) printf("Failed to open main window!!!\n");

	screen = SDL_GetWindowSurface(window);
	//screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, bpp, flags);
	if (!screen)
	{
		fprintf(stderr, "Failed to open window!\n");
		exit(-1);
	}

	border = screen->clip_rect;
	SDL_ShowCursor(0);

    SDL_RWops *rwop = SDL_RWFromConstMem(imageRAW, sizeof(imageRAW)); 
	tiles_bmp = IMG_Load_RW(rwop, 1); //SDL_LoadBMP("tiles.bmp");
	if (!tiles_bmp)
	{
		fprintf(stderr, "Could not load graphics!\n");
		exit(-1);
	}
	tiles = SDL_ConvertSurfaceFormat(tiles_bmp, SDL_GetWindowPixelFormat(window), 0);
	otiles = SDL_ConvertSurfaceFormat(tiles_bmp, SDL_GetWindowPixelFormat(window), 0);
	SDL_FreeSurface(tiles_bmp);

	/* Set colorkey for non-opaque tiles to bright magenta */
	SDL_SetColorKey(tiles, SDL_RLEACCEL,
			SDL_MapRGB(tiles->format,255,0,255)); //SDL_SRCCOLORKEY
	//if (alpha)
	//	SDL_SetAlpha(tiles, SDL_RLEACCEL, alpha); //SDL_SRCALPHA

	if (num_of_layers > 1)
	{
		/* Assign maps and tile palettes to parallax layers */
		layer_init(&layers[0], &foreground_map, tiles, otiles);
		for(i = 1; i < num_of_layers - 1; ++i)
			if( (i & 1) && use_planets)
				layer_init(&layers[i], &middle_map,
						tiles, otiles);
			else
				layer_init(&layers[i], &foreground_map,
						tiles, otiles);
		layer_init(&layers[num_of_layers-1], &background_map,
				tiles, otiles);
		/*
		 * Set up the depth order for the
		 * recursive rendering algorithm.
		 */
		for(i = 0; i < num_of_layers - 1; ++i)
			layer_next(&layers[i], &layers[i+1]);
	}
	else
		layer_init(&layers[0], &single_map, tiles, otiles);

	
	if(bounce_around && (num_of_layers > 1))
	{
		for(i = 0; i < num_of_layers - 1; ++i)
		{
			float a = 1.0 + i * 2.0 * 3.1415 / num_of_layers;
			float v = 200.0 / (i+1);
			layer_vel(&layers[i], v * cos(a), v * sin(a));
			if(!wrap)
				layer_limit_bounce(&layers[i]);
		}
	}
	else
	{
		/* Set foreground scrolling speed and enable "bounce mode" */
		layer_vel(&layers[0], FOREGROUND_VEL_X, FOREGROUND_VEL_Y);
		if(!wrap)
			layer_limit_bounce(&layers[0]);

		/* Link all intermediate levels to the foreground layer */
		for(i = 1; i < num_of_layers - 1; ++i)
			layer_link(&layers[i], &layers[0], 1.5 / (float)(i+1));
	}

	/* Get initial tick for time calculation */
	tick1 = SDL_GetTicks();

	while (SDL_PollEvent(&event) >= 0)
	{
		/* Click to exit */
		if (event.type == SDL_MOUSEBUTTONDOWN)
			break;

		/*if (event.type & (SDL_KEYUP | SDL_KEYDOWN))
		{
			Uint16	*x, *y;
			const Uint8	*keys = SDL_GetKeyboardState(NULL);
			if(keys[SDLK_ESCAPE])
				break;

			if(keys[SDLK_LSHIFT] || keys[SDLK_RSHIFT])
			{
				x = &border.w;
				y = &border.h;
			}
			else
			{
				x = &border.x;
				y = &border.y;
			}

			if(keys[SDLK_UP])
				-- *y;
			else if(keys[SDLK_DOWN])
				++ *y;
			if(keys[SDLK_LEFT])
				-- *x;
			else if(keys[SDLK_RIGHT])
				++ *x;
		}*/

		/* calculate time since last update */
		tick2 = SDL_GetTicks();
		dt = (tick2 - tick1) * 0.001f;
		tick1 = tick2;

		/* Set background velocity */
		if(num_of_layers > 1)
			layer_vel(&layers[num_of_layers - 1],
					sin(tick1 * 0.00011f) * BACKGROUND_VEL,
					cos(tick1 * 0.00013f) * BACKGROUND_VEL);

		/* Animate all layers */
		for(i = 0; i < num_of_layers; ++i)
			layer_animate(&layers[i], dt);

		/* Reset rendering statistics */
		for(i = 0; i < num_of_layers; ++i)
			layer_reset_stats(&layers[i]);

		/* Render layers (recursive!) */
		layer_render(&layers[0], screen, &border);

		total_blits = 0;
		total_recursions = 0;
		total_pixels = 0;
		if(verbose >= 1)
			printf("\nlayer    blits recursions pixels\n");
		if(verbose >= 3)
			for(i = 0; i < num_of_layers; ++i)
			{
				printf("%4d %8d %8d %8d\n",
						i,
						layers[i].blits,
						layers[i].recursions,
						layers[i].pixels);
			}

		for(i = 0; i < num_of_layers; ++i)
		{
			total_blits += layers[i].blits;
			total_recursions += layers[i].recursions;
			total_pixels += layers[i].pixels;
		}
		if(total_blits > peak_blits)
			peak_blits = total_blits;
		if(total_recursions > peak_recursions)
			peak_recursions = total_recursions;
		if(total_pixels > peak_pixels)
			peak_pixels = total_pixels;

		if(verbose >= 2)
			printf("TOTAL:%8d %8d %8d\n",
					total_blits,
					total_recursions,
					total_pixels);
		if(verbose >= 1)
			printf("PEAK:%8d %8d %8d\n",
					peak_blits,
					peak_recursions,
					peak_pixels);

		/* Draw "title" tile in upper left corner */
		SDL_SetClipRect(screen, NULL);
		draw_tile(screen, tiles, 2, 2, '4');

		/* make changes visible */
		//SDL_Flip(screen);
        SDL_UpdateWindowSurface(window);

		/* let operating system breath */
		SDL_Delay(1);
	}

	printf("Statistics: (All figures per rendered frame.)\n"
			"\tblits = %d\n"
			"\trecursions = %d\n"
			"\tpixels = %d\n",
			peak_blits,
			peak_recursions,
			peak_pixels);
	SDL_FreeSurface(tiles);
	exit(0);
}


/*----------------------------------------------------------
	layer_t functions
----------------------------------------------------------*/

/* Initialivze layer; set up map and tile graphics data. */
void layer_init(layer_t *lr, map_data_t *map,
		SDL_Surface *tiles, SDL_Surface *opaque_tiles)
{
	lr->next = NULL;
	lr->pos_x = lr->pos_y = 0.0;
	lr->vel_x = lr->vel_y = 0.0;
	lr->map = map;
	lr->tiles = tiles;
	lr->opaque_tiles = opaque_tiles;
	lr->link = NULL;
}

/* Tell a layer which layer is next, or under this layer */
void layer_next(layer_t *lr, layer_t *next_lr)
{
	lr->next = next_lr;
}

/* Set position */
void layer_pos(layer_t *lr, float x, float y)
{
	lr->pos_x = x;
	lr->pos_y = y;
}

/* Set velocity */
void layer_vel(layer_t *lr, float x, float y)
{
	lr->vel_x = x;
	lr->vel_y = y;
}

void __do_limit_bounce(layer_t *lr)
{
	int	maxx = MAP_W * TILE_W - SCREEN_W;
	int	maxy = MAP_H * TILE_H - SCREEN_H;

	if(lr->pos_x >= maxx)
	{
		/* v.out = - v.in */
		lr->vel_x = -lr->vel_x;
		/*
		 * Mirror over right limit. We need to do this
		 * to be totally accurate, as we're in a time
		 * discreet system! Ain't that obvious...? ;-)
		 */
		lr->pos_x = maxx * 2 - lr->pos_x;
	}
	else if(lr->pos_x <= 0)
	{
		/* Basic physics again... */
		lr->vel_x = -lr->vel_x;
		/* Mirror over left limit */
		lr->pos_x = -lr->pos_x;
	}

	if(lr->pos_y >= maxy)
	{
		lr->vel_y = -lr->vel_y;
		lr->pos_y = maxy * 2 - lr->pos_y;
	}
	else if(lr->pos_y <= 0)
	{
		lr->vel_y = -lr->vel_y;
		lr->pos_y = -lr->pos_y;
	}
}

/* Update animation (apply the velocity, that is) */
void layer_animate(layer_t *lr, float dt)
{
	if(lr->flags & TL_LINKED)
	{
		lr->pos_x = lr->link->pos_x * lr->ratio;
		lr->pos_y = lr->link->pos_y * lr->ratio;
	}
	else
	{
		lr->pos_x += dt * lr->vel_x;
		lr->pos_y += dt * lr->vel_y;
		if(lr->flags & TL_LIMIT_BOUNCE)
			__do_limit_bounce(lr);
	}
}

/* Bounce at map limits */

void layer_limit_bounce(layer_t *lr)
{
	lr->flags |= TL_LIMIT_BOUNCE;
}

/*
 * Link the position of this layer to another layer, w/ scale ratio
 */
void layer_link(layer_t *lr, layer_t *to_lr, float ratio)
{
	lr->flags |= TL_LINKED;
	lr->link = to_lr;
	lr->ratio = ratio;
}

/*
 * Render layer to the specified surface,
 * clipping to the specified rectangle
 *
 * This version is slightly improved over the 
 * one in "Parallax 3"; it combines horizontal
 * runs of transparent and partially transparent
 * tiles before recursing down.
 */
void layer_render(layer_t *lr, SDL_Surface *screen, SDL_Rect *rect)
{
	int		max_x, max_y;
	int		map_pos_x, map_pos_y;
	int		mx, my, mx_start;
	int		fine_x, fine_y;
	SDL_Rect	pos;
	SDL_Rect	local_clip;

	/*
	 * Set up clipping
	 * (Note that we must first clip "rect" to the
	 * current cliprect of the screen - or we'll screw
	 * clipping up as soon as we have more than two
	 * layers!)
	 */
	if(rect)
	{
		pos = screen->clip_rect;
		local_clip = *rect;
		/* Convert to (x2,y2) */
		pos.w += pos.x;
		pos.h += pos.y;
		local_clip.w += local_clip.x;
		local_clip.h += local_clip.y;
		if(local_clip.x < pos.x)
			local_clip.x = pos.x;
		if(local_clip.y < pos.y)
			local_clip.y = pos.y;
		if(local_clip.w > pos.w)
			local_clip.w = pos.w;
		if(local_clip.h > pos.h)
			local_clip.h = pos.h;
		/* Convert result back to w, h */
		local_clip.w -= local_clip.x;
		local_clip.h -= local_clip.y;
		/* Check if we actually have an area left! */
		if( (local_clip.w <= 0) || (local_clip.h <= 0) )
			return;
		/* Set the final clip rect */
		SDL_SetClipRect(screen, &local_clip);
	}
	else
	{
		SDL_SetClipRect(screen, NULL);
		local_clip = screen->clip_rect;
	}

	/* Position of clip rect in map space */
	map_pos_x = (int)lr->pos_x + screen->clip_rect.x;
	map_pos_y = (int)lr->pos_y + screen->clip_rect.y;

	/* The calculations would break with negative map coords... */
	if(map_pos_x < 0)
		map_pos_x += MAP_W*TILE_W*(-map_pos_x/(MAP_W*TILE_W)+1);
	if(map_pos_y < 0)
		map_pos_y += MAP_H*TILE_H*(-map_pos_y/(MAP_H*TILE_H)+1);

	/* Position on map in tiles */
	mx = map_pos_x / TILE_W;
	my = map_pos_y / TILE_H;

	/* Fine position - pixel offset; up to (1 tile - 1 pixel) */
	fine_x = map_pos_x % TILE_W;
	fine_y = map_pos_y % TILE_H;

	/* Draw all visible tiles */
	max_x = screen->clip_rect.x + screen->clip_rect.w;
	max_y = screen->clip_rect.y + screen->clip_rect.h;
	mx_start = mx;
	pos.h = TILE_H;
	for(pos.y = screen->clip_rect.y - fine_y;
			pos.y < max_y; pos.y += TILE_H)
	{
		mx = mx_start;
		my %= MAP_H;
		for(pos.x = screen->clip_rect.x - fine_x;
				pos.x < max_x; )
		{
			int	tile, tm, run_w;
			mx %= MAP_W;
			tile = (*lr->map)[my][mx];
			tm = tile_mode(tile);

			/* Calculate run length
			 * ('tm' will tell what kind of run it is)
			 */
			run_w = 1;
			if(detect_runs)
			  while(pos.x + run_w*TILE_W < max_x)
			  {
				int tt = (*lr->map)[my]
						[(mx + run_w) % MAP_W];
				tt = tile_mode(tt);
				if(tt != TM_OPAQUE)
					tt = TM_EMPTY;
				if(tm != tt)
					break;
				++run_w;
			  }

			/* Recurse to next layer */
			if((tm != TM_OPAQUE) && lr->next)
			{
				++lr->recursions;
				pos.w = run_w * TILE_W;
				/* !!! Recursive call !!! !*/
				layer_render(lr->next, screen, &pos);
				SDL_SetClipRect(screen, &local_clip);
			}

			/* Render our tiles */
			pos.w = TILE_W;
			while(run_w--)
			{
				mx %= MAP_W;
				tile = (*lr->map)[my][mx];
				tm = tile_mode(tile);
				if(tm != TM_EMPTY)
				{
					SDL_Surface *tiles;
					if(tm == TM_OPAQUE)
						tiles = lr->opaque_tiles;
					else
						tiles = lr->tiles;
					++lr->blits;
					lr->pixels += draw_tile(screen, tiles,
							pos.x, pos.y, tile);
				}
				++mx;
				pos.x += TILE_W;
			}
		}
		++my;
	}
}

void layer_reset_stats(layer_t *lr)
{
	lr->blits = 0;
	lr->recursions = 0;
	lr->pixels = 0;
}


/*----------------------------------------------------------
	EOF
----------------------------------------------------------*/
