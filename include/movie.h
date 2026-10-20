#ifndef MOVIE_H
#define MOVIE_H

/*
 * The movie player's vocabulary: Movie_GetStatus's results (the Sofdec
 * player's status, as the getStat call returns it) and the display modes of
 * a movie's entry in gMovieDescs.
 */
#define MOVIE_STATUS_STOPPED 0      /* also when no player exists */
#define MOVIE_STATUS_PREPARING 1    /* stopped after MOVIE_PREPARE_FRAMES */
#define MOVIE_STATUS_PLAYING 2      /* each frame is drawn */
#define MOVIE_STATUS_FINISHED 3     /* Movie_Update asks the player to stop */
#define MOVIE_STATUS_ERROR 4        /* Movie_Update stops and frees the player */

#define MOVIE_MODE_FULL 0           /* drawn 512 by 448 */
#define MOVIE_MODE_SMALL 1          /* drawn 256 by 224 */
#define MOVIE_MODE_WIDE 2           /* drawn 512 by 432, 8 lines down */

#define MOVIE_PREPARE_FRAMES 300    /* frames: 5 s */

#endif /* MOVIE_H */
