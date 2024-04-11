#include "main.h"

#define TRUE 1
#define FALSE 0;


#define SWAP(x, y) (x ^= y ^= x ^= y)
#define ABS(x) ((x >= 0) ? x : -x)

static int _w = -1;
static int _h = -1;
static int _framecount = 0;

static int _s = 0;
static int _sizebytes = 0;
static unsigned char *_oi;

unsigned int _color = 0xffffffff;

static MATRIX3D *_rx = NULL;
static MATRIX3D *_ry = NULL;
static MATRIX3D *_rz = NULL;
static MATRIX3D *_scale = NULL;
static MATRIX3D *_rotation = NULL;
static MATRIX3D *_world = NULL;
static MATRIX3D *_view = NULL;
static MATRIX3D *_projection = NULL;
static MATRIX3D *_wvp = NULL;
static VECTOR3D *_lightpos = NULL;
static VECTOR3D *_lightdir = NULL;

static float _distance = 6.0f;

static float _cubevertices[] = {
    -1.0, -1.0,  1.0,
	 1.0, -1.0,  1.0,
	 1.0,  1.0,  1.0,
	-1.0,  1.0,  1.0,
	-1.0, -1.0, -1.0,
	 1.0, -1.0, -1.0,
	 1.0,  1.0, -1.0,
    -1.0,  1.0, -1.0
};

// ! Not correct
static float _cubenormals[] = {
	 0.0,  0.0,  1.0,
     0.0,  0.0, -1.0,
	 1.0,  1.0,  1.0,
	-1.0,  1.0,  1.0,
	-1.0, -1.0, -1.0,
     1.0, -1.0, -1.0,
	 1.0,  1.0, -1.0,
	-1.0,  1.0, -1.0
};

static float _cubeindices[] = {
	0, 1, 2, 2, 3, 0,
	3, 2, 6, 6, 7, 3,
	7, 6, 5, 5, 4, 7,
	4, 0, 3, 3, 7, 4,
	0, 1, 5, 5, 4, 0,
	1, 5, 6, 6, 2, 1
};


void clearcolor(unsigned int color) {
    int i;
    unsigned char ina = (color >> 24) & 0xff;
    unsigned char inr = (color >> 16) & 0xff;
    unsigned char ing = (color >> 8) & 0xff;
    unsigned char inb = color & 0xff;

    for(i = 0; i < _s; i++) {
       _oi[i*4] = inr;
       _oi[i*4] = ing;
       _oi[i*4] = inb;
       _oi[i*4] = ina;
    }
}

void blockblend(int x, int y, int w, int h, unsigned int color) {
    int i, j;
    int sx = x;
    int sy = y;
    int ex = x+w;
    int ey = y+h;
    unsigned char ina = (color >> 24) & 0xff;
    unsigned char ininva = 0xff - ina;
    unsigned char inr = (color >> 16) & 0xff;
    unsigned char ing = (color >> 8) & 0xff;
    unsigned char inb = color & 0xff;

    if(sx < 0) sx = 0;
    if(sy < 0) sy = 0;

    for(j = sy; j < ey && j < _h; j++) for(i = sx; i < ex && i < _w; i++) {
       unsigned char orgr = _oi[(j*_w+i)*4];
       unsigned char orgg = _oi[(j*_w+i)*4+1];
       unsigned char orgb = _oi[(j*_w+i)*4+2];
       unsigned char outr = (orgr * ininva + inr * ina) >> 8;
       unsigned char outg = (orgg * ininva + ing * ina) >> 8;
       unsigned char outb = (orgb * ininva + inb * ina) >> 8;
       unsigned char outa = 0xff;
       _oi[(j*_w+i)*4] = outr;
       _oi[(j*_w+i)*4+1] = outg;
       _oi[(j*_w+i)*4+2] = outb;
       _oi[(j*_w+i)*4+3] = outa;
    }
}

void coloroutsync(int ofset) {
    int i, j, k;


    for(k = 0; k < ofset; k++) {
        for(j = 0; j < _h; j++) {
            unsigned char rfirst = _oi[(j*_w)*4];
            unsigned char blast = _oi[(j*_w+_w-1)*4];
            for(i = 0; i < _w-1; i++) {
                _oi[(j*_w+i)*4] = _oi[(j*_w+i+1)*4];
                _oi[(j*_w+_w-1-i)*4+2] = _oi[(j*_w+_w-1-i-1)*4+2];
            }
            _oi[(j*_w+i)*4]  = rfirst;
            _oi[(j*_w+_w-1-i)*4+2] = blast;
        }
    }
}

void pixelblend(int x, int y, int color)
{
    if(x < 0 || y < 0 || x >= _w || y >= _h)
        return;

    unsigned char ina = (color >> 24) & 0xff;
    unsigned char ininva = 0xff - ina;
    unsigned char inr = (color >> 16) & 0xff;
    unsigned char ing = (color >> 8) & 0xff;
    unsigned char inb = color & 0xff;
    int i = (y*_w+x)*4;

    _oi[i] = (_oi[i] * ininva + inr * ina) >> 8;
    _oi[i+1] = (_oi[i+1] * ininva + ing * ina) >> 8;
    _oi[i+2] = (_oi[i+2] * ininva + inb * ina) >> 8;
    _oi[i+3] = 0xff;
}

void lineblend(VECTOR2D *v1, VECTOR2D *v2, int color)
{
    int x0 = (int)(v1->x+0.5f);
    int y0 = (int)(v1->y+0.5f);
    int x1 = (int)(v2->x+0.5f);
    int y1 = (int)(v2->y+0.5f);


    int Dx = x1 - x0;
    int Dy = y1 - y0;
    int steep = (abs(Dy) >= abs(Dx));
    if (steep) {
        SWAP(x0, y0);
        SWAP(x1, y1);
        // recompute Dx, Dy after swap
        Dx = x1 - x0;
        Dy = y1 - y0;
    }
    int xstep = 1;
    if (Dx < 0) {
        xstep = -1;
        Dx = -Dx;
    }
    int ystep = 1;
    if (Dy < 0) {
        ystep = -1;
        Dy = -Dy;
    }
    int TwoDy = 2*Dy;
    int TwoDyTwoDx = TwoDy - 2*Dx; // 2*Dy - 2*Dx
    int E = TwoDy - Dx; //2*Dy - Dx
    int y = y0;
    int xDraw, yDraw;
    for (int x = x0; x != x1; x += xstep) {
        if (steep) {
            xDraw = y;
            yDraw = x;
        } else {
            xDraw = x;
            yDraw = y;
        }
        // plot
        pixelblend(xDraw, yDraw, color);

        // next
        if (E > 0) {
            E += TwoDyTwoDx; //E += 2*Dy - 2*Dx;
            y = y + ystep;
        } else {
            E += TwoDy; //E += 2*Dy;
        }
    }
}



void cubecreate() {
        VECTOR3D *eye;
        VECTOR3D *target;
        VECTOR3D *up;

        _lightpos = vector3d_create(-50.f, -50.f, -50.f);
        _lightdir = vector3d_create(1.f, 1.f, 1.f);

        _scale = matrix3d_scale(1.f, 1.f, 1.f);

        eye = vector3d_create(0.0f, 0.0f, _distance);
        target = vector3d_create(0.0f, 0.0f, _distance-3.0);
        up = vector3d_create(0.0f, 1.0f, 0.0f);
		_view = matrix3d_lookat(eye, target, up);
        _projection = matrix3d_perspective(45, (float)_w/(float)_h, 1.0, 1000.0);


        vector3d_destroy(eye);
        vector3d_destroy(target);
        vector3d_destroy(up);
}

void cubedestroy() {
        if(_scale)
            matrix3d_destroy(_scale);
        if(_rotation)
            matrix3d_destroy(_rotation);
        if(_view)
            matrix3d_destroy(_view);
        if(_projection)
            matrix3d_destroy(_projection);
        if(_lightpos)
            vector3d_destroy(_lightpos);
        if(_lightdir)
            vector3d_destroy(_lightdir);
}

void cuberender(int wireframe) {
    int i;
    int index;
    VECTOR3D *vi1, *vi2, *vi3;
    VECTOR3D *vo1, *vo2, *vo3;
    VECTOR2D *t1, *t2, *t3;
    VECTOR3D *normal;

    for(i = 0; i < 36; i += 3)
    {
		index = _cubeindices[i]*3;
		vi1 = vector3d_create(
            _cubevertices[index],
            _cubevertices[index+1],
            _cubevertices[index+2]
        );
		index = _cubeindices[i+1]*3;
		vi2 = vector3d_create(
            _cubevertices[index],
            _cubevertices[index+1],
            _cubevertices[index+2]
        );
		index = _cubeindices[i+2]*3;
		vi3 = vector3d_create(
            _cubevertices[index],
            _cubevertices[index+1],
            _cubevertices[index+2]
        );
        index = _cubeindices[i/3]*3;
        normal = vector3d_create(_cubenormals[index], _cubenormals[index], _cubenormals[index]);


        vo1 = matrix3d_vector(_wvp, vi1);
        vo2 = matrix3d_vector(_wvp, vi2);
        vo3 = matrix3d_vector(_wvp, vi3);


		t1 = vector2d_create(
            _w * (vo1->x + 1.0) / 2.0,
            _h * (0.0 - vo1->y + 1.0) / 2.0
        );
		t2 = vector2d_create(
            _w * (vo2->x + 1.0) / 2.0,
            _h * (0.0 - vo2->y + 1.0) / 2.0
		);
        t3 = vector2d_create(
            _w * (vo3->x + 1.0) / 2.0,
            _h * (0.0 - vo3->y + 1.0) / 2.0
		);

        if(wireframe) {
            lineblend(t1, t2, _color);
            lineblend(t2, t3, _color);
            lineblend(t1, t3, _color);
        }
        //else
            //triangleblend(t1, t2, t3, _color);

        vector3d_destroy(vi1);
        vector3d_destroy(vi2);
        vector3d_destroy(vi3);
        vector3d_destroy(vo1);
        vector3d_destroy(vo2);
        vector3d_destroy(vo3);
        vector2d_destroy(t1);
        vector2d_destroy(t2);
        vector2d_destroy(t3);
        vector3d_destroy(normal);
    }
}

int filtercreate(int fps) {
    cubecreate();
    return TRUE;
}

int filterdestroy() {
    cubedestroy();
    return TRUE;
}

int filterstep(unsigned char *buffer, int w, int h, unsigned int color, char *text, int64_t framecount)
{
    _framecount = framecount;
    _oi = buffer;
    _color = color;

    if(w != _w || h != _h)
    {
        cubedestroy();
        _w = w;
        _h = h;
        _s = _w * _h;
        _sizebytes = _s * 4;
        cubecreate();
    }

    _rz = matrix3d_rotationz(_framecount*0.01f);
    _ry = matrix3d_rotationy(_framecount*0.01f);
    _rx = matrix3d_rotationx(_framecount*0.01f);
    _rotation = matrix3d_multiply3(_rx, _ry, _rz);
    _world = matrix3d_multiply(_scale, _rotation);
    _wvp = matrix3d_multiply3(_world, _view, _projection);

    cuberender(0);
    cuberender(1);


    matrix3d_destroy(_rx);
    matrix3d_destroy(_ry);
    matrix3d_destroy(_rz);
    matrix3d_destroy(_rotation);
    matrix3d_destroy(_world);
    matrix3d_destroy(_wvp);

    return TRUE;
}

