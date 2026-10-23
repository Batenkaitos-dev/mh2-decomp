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

/* One movie: an entry of gMovieDescs (16 bytes). */
typedef struct MovieDesc {
    s32 entry;          /* 0x0: file in AFS02 */
    s32 ftype;          /* 0x4 */
    u16 maxKbps;        /* 0x8 */
    u16 width;          /* 0xA */
    u16 height;         /* 0xC */
    u16 mode;           /* 0xE: MOVIE_MODE_*: how it is drawn */
} MovieDesc;

void Movie_AllocFrame(s32 width, s32 height);
void Movie_FreeFrame(void);
u32 Movie_BuildUploadChain(u32 chain, void *buf, s32 tbp, s32 psm, s32 width, s32 height);
void Movie_SetupBlockPackets(u32 chain, u32 *pixels, s32 width, s32 height, s32 skip);
u32 MovieSfd_FramePacket(u32 packet, s32 x, s32 y, s32 z, s32 width, s32 height);
void MovieSfd_Draw(u32 *pixels, s32 width, s32 height, s32 setupOnly);
void MovieSfd_Create(void *arg);
void Movie_PrePlay(void);
s32 Movie_Play(s32 id);
void Movie_SetPosition(s32 x, s32 y);
void Movie_RequestStop(void);
void Movie_Stop(void);
s32 Movie_GetStatus(void);
s32 Movie_Update(void);

#endif /* MOVIE_H */
