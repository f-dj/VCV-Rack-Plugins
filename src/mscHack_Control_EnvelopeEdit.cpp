#include "mscHack.hpp"
#include "window.hpp"

//-----------------------------------------------------
// Procedure:   constructor
//-----------------------------------------------------
Widget_EnvelopeEdit::Widget_EnvelopeEdit( int x, int y, int w, int h, int handleSize, void *pClass, EnvelopeEditCALLBACK *pCallback, int nchannels )
{
    int ch, hd;

    if( !pClass || !pCallback )
        return;

    m_pClass = pClass;
    m_pCallback = pCallback;
    m_handleSize = handleSize;
    m_MaxChannels = nchannels;

	box.pos = Vec( x, y );
    box.size = Vec( w, h );

    // calc division size (16 divisions, 4 beats x 4 quarter)
    m_divw = ( ( (float)w - (float)ENVELOPE_HANDLES) / (float)ENVELOPE_DIVISIONS ) + 1.0f;

    m_handleSizeD2 = ( (float)handleSize / 2.0f );

    // handles
    for( ch = 0; ch < MAX_ENVELOPE_CHANNELS; ch++ )
    {
    	m_EnvData[ ch ].Init( EnvelopeData::MODE_LOOP, EnvelopeData::RANGE_0to5, false, m_divw );

        for( hd = 0; hd < ENVELOPE_HANDLES; hd++ )
            m_HandleCol[ hd ].dwCol = 0xFFFFFF;
    }

    recalcLine( -1, 0 );

    m_BeatLen = (int)engineGetSampleRate();

    m_bInitialized  = true;
}

//-----------------------------------------------------
// Procedure:   Val2y
//-----------------------------------------------------
float Widget_EnvelopeEdit::Val2y( float fval )
{
    return clamp( box.size.y - ( fval * box.size.y ), 0.0f, box.size.y );
}

//-----------------------------------------------------
// Procedure:   y2Val
//-----------------------------------------------------
float Widget_EnvelopeEdit::y2Val( float fy )
{
    return clamp( 1.0 - (fy / box.size.y ), 0.0f, 1.0f );
}

//-----------------------------------------------------
// Procedure:   recalcLine
//-----------------------------------------------------
void Widget_EnvelopeEdit::recalcLine( int chin, int handle )
{
    // calc all lines
    if( chin == -1 )
    {
        for( int ch = 0; ch < MAX_ENVELOPE_CHANNELS; ch++ )
        {
        	m_EnvData[ ch ].recalcLine( -1 );
        }
    }
    // calc line before and line after handle
    else
    {
    	m_EnvData[ chin ].recalcLine( handle );
    }
}

//-----------------------------------------------------
// Procedure:   setPos
//-----------------------------------------------------
void Widget_EnvelopeEdit::setView( int ch )
{
    if( !m_bInitialized && ch < MAX_ENVELOPE_CHANNELS && ch >= 0 )
        return;

    m_currentChannel = ch;
}

//-----------------------------------------------------
// Procedure:   resetVal
//-----------------------------------------------------
void Widget_EnvelopeEdit::resetValAll( int ch, float val )
{
    if( !m_bInitialized && ch < MAX_ENVELOPE_CHANNELS && ch >= 0 )
        return;

    m_EnvData[ ch ].resetValAll( val );
}

//-----------------------------------------------------
// Procedure:   setVal
//-----------------------------------------------------
void Widget_EnvelopeEdit::setVal( int ch, int handle, float val )
{
    if( !m_bInitialized && ch < MAX_ENVELOPE_CHANNELS && ch >= 0 )
        return;

    m_EnvData[ ch ].setVal( handle, val );
}

//-----------------------------------------------------
// Procedure:   setRange
//-----------------------------------------------------
void Widget_EnvelopeEdit::setRange( int ch, int Range )
{
    if( !m_bInitialized && ch < MAX_ENVELOPE_CHANNELS && ch >= 0 )
        return;

    m_EnvData[ ch ].m_Range = Range;
}

//-----------------------------------------------------
// Procedure:   setMode
//-----------------------------------------------------
void Widget_EnvelopeEdit::setMode( int ch, int Mode )
{
    if( !m_bInitialized && ch < MAX_ENVELOPE_CHANNELS && ch >= 0 )
        return;

    m_EnvData[ ch ].setMode( Mode );
}

//-----------------------------------------------------
// Procedure:   setGateMode
//-----------------------------------------------------
void Widget_EnvelopeEdit::setGateMode( int ch, bool bGate )
{
    if( !m_bInitialized && ch < MAX_ENVELOPE_CHANNELS && ch >= 0 )
        return;

    m_EnvData[ ch ].m_bGateMode = bGate;
}

//-----------------------------------------------------
// Procedure:   setTimeDiv
//-----------------------------------------------------
void Widget_EnvelopeEdit::setTimeDiv( int ch, int timediv )
{
    if( !m_bInitialized && ch < MAX_ENVELOPE_CHANNELS && ch >= 0 )
        return;

    m_TimeDiv[ ch ] = timediv;
    setBeatLen( m_BeatLen );
}
//-----------------------------------------------------
// Procedure:   setDataAll
//-----------------------------------------------------
int Widget_EnvelopeEdit::getPos( int ch )
{
    return (int)( m_EnvData[ ch ].m_Clock.fpos * 10000.0f );
}

//-----------------------------------------------------
// Procedure:   setDataAll
//-----------------------------------------------------
void Widget_EnvelopeEdit::setPos( int ch, int pos )
{
	m_EnvData[ ch ].m_Clock.fpos = (float)pos / 10000.0f;
}

//-----------------------------------------------------
// Procedure:   setDataAll
//-----------------------------------------------------
void Widget_EnvelopeEdit::setDataAll( int *pint )
{
    int i, j, count = 0;

    if( !m_bInitialized )
        return;

    for( i = 0; i < MAX_ENVELOPE_CHANNELS; i++ )
    {
        for( j = 0; j < ENVELOPE_HANDLES; j++ )
        {
        	m_EnvData[ i ].m_HandleVal[ j ] = clamp( (float)pint[ count++ ] / 10000.0f, 0.0f, 1.0f );
        }
    }

    // recalc all lines
    recalcLine( -1, 0 );
}

//-----------------------------------------------------
// Procedure:   getDataAll
//-----------------------------------------------------
void Widget_EnvelopeEdit::getDataAll( int *pint )
{
    int i, j, count = 0;

    if( !m_bInitialized )
        return;

    for( i = 0; i < MAX_ENVELOPE_CHANNELS; i++ )
    {
        for( j = 0; j < ENVELOPE_HANDLES; j++ )
        {
            pint[ count++ ] = (int)( m_EnvData[ i ].m_HandleVal[ j ] * 10000.0 );
        }
    }
}

//-----------------------------------------------------
// Procedure:   draw
//-----------------------------------------------------
int hdivs[ EnvelopeData::nRANGES ] = { 6, 11, 11, 21, 2, 13 };
void Widget_EnvelopeEdit::draw( NVGcontext *vg )
{
    int h, hlines;
    float x, y, divsize;
    float linewidth = 1.0;

    if( !m_bInitialized )
        return;

    // fill bg
    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, box.size.x-1, box.size.y-1 );
    //nvgFillColor(vg, nvgRGBA(40,40,40,255));
    nvgFillColor(vg, nvgRGBA(57, 10, 10,255));
    nvgFill(vg);


    nvgStrokeWidth( vg, linewidth );
    nvgStrokeColor( vg, nvgRGBA( 60, 60, 60, 255 ) );
    // draw bounding line
    /*nvgBeginPath( vg );
    nvgMoveTo( vg, 0, 0 );
    nvgLineTo( vg, box.size.x - 1, 0 );
    nvgLineTo( vg, box.size.x - 1, box.size.y - 1 );
    nvgLineTo( vg, 0, box.size.y - 1 );
    nvgClosePath( vg );
    nvgStroke( vg );*/

    x = 0.0;

    // draw vertical lines 
    for( h = 0; h < ENVELOPE_HANDLES; h++ )
    {
        nvgBeginPath(vg);
        nvgMoveTo( vg, x, 0 );
        nvgLineTo( vg, x, box.size.y - 1 );
        nvgStroke( vg );
        x += m_divw;
    }

    y = 0.0;

    hlines = hdivs[ m_EnvData[ m_currentChannel ].m_Range ];

    divsize = box.size.y / (float)( hlines - 1 );

    // draw horizontal lines
    for( h = 0; h < hlines; h++ )
    {
        if( h == hlines / 2 && ( m_EnvData[ m_currentChannel ].m_Range == EnvelopeData::RANGE_n5to5 || m_EnvData[ m_currentChannel ].m_Range == EnvelopeData::RANGE_n10to10 || m_EnvData[ m_currentChannel ].m_Range == EnvelopeData::RANGE_Audio ) )
            nvgStrokeColor( vg, nvgRGBA( 255, 255, 255, 255 ) );
        else
            nvgStrokeColor( vg, nvgRGBA( 80, 80, 80, 255 ) );

        nvgBeginPath(vg);
        nvgMoveTo( vg, 0, y );
        nvgLineTo( vg, box.size.x - 1, y );
        nvgStroke( vg );
        y += divsize;
    }

    if( m_EnvData[ m_currentChannel ].m_bGateMode )
    {
        // draw rects
        for( h = 0; h < ENVELOPE_DIVISIONS; h++ )
        {
            nvgBeginPath(vg);
            nvgRect( vg, h * m_divw, ( 1.0f - m_EnvData[ m_currentChannel ].m_HandleVal[ h ] ) * box.size.y, m_divw, box.size.y * m_EnvData[ m_currentChannel ].m_HandleVal[ h ] );
            nvgFillColor(vg, nvgRGBA( 157, 100, 100, 128 ) );
            nvgFill(vg);
        }
    }
    else
    {
        nvgStrokeColor( vg, nvgRGBA( 255, 255, 255, 255 ) );
        nvgBeginPath(vg);

        x = 0;
        nvgMoveTo( vg, x, ( 1.0f - m_EnvData[ m_currentChannel ].m_HandleVal[ 0 ] ) * box.size.y );

        // draw lines
        for( h = 1; h < ENVELOPE_HANDLES; h++ )
        {
            x += m_divw;
            nvgLineTo( vg, x, ( 1.0f - m_EnvData[ m_currentChannel ].m_HandleVal[ h ] ) * box.size.y );
        }

        nvgStroke( vg );

        x = 0;
        // draw handles
        for( h = 0; h < ENVELOPE_DIVISIONS + 1; h++ )
        {
            nvgBeginPath(vg);
            y = ( 1.0f - m_EnvData[ m_currentChannel ].m_HandleVal[ h ] ) * box.size.y;
            nvgRect( vg, x - m_handleSizeD2, y - m_handleSizeD2, m_handleSize, m_handleSize );
            nvgFillColor(vg, nvgRGBA( m_HandleCol[ h ].Col[ 2 ], m_HandleCol[ h ].Col[ 1 ], m_HandleCol[ h ].Col[ 0 ],255 ) );
            nvgFill(vg);

             x += m_divw;
        }
    }

    // draw indicator line
    if( m_EnvData[ m_currentChannel ].m_Range != EnvelopeData::RANGE_Audio )
    {
        nvgStrokeColor( vg, nvgRGBA( 255, 255, 255, 80 ) );
        nvgBeginPath(vg);
        nvgMoveTo( vg, m_EnvData[ m_currentChannel ].m_fIndicator * box.size.x, 0 );
        nvgLineTo( vg, m_EnvData[ m_currentChannel ].m_fIndicator * box.size.x, box.size.y );
        nvgStroke( vg );
    }
}

//-----------------------------------------------------
// Procedure:   onMouseDown
//-----------------------------------------------------
void Widget_EnvelopeEdit::onMouseDown( EventMouseDown &e )
{
    int index;

    m_Drag = -1;
    e.consumed = false;

    OpaqueWidget::onMouseDown( e );

    if( !m_bInitialized )
        return;

    if( e.button == 0 )
    {
        if( !m_bDraw )
            windowCursorLock();

        if( m_EnvData[ m_currentChannel ].m_bGateMode )
            m_Drag = clamp( ( e.pos.x / box.size.x ) * (float)ENVELOPE_DIVISIONS, 0.0f, (float)(ENVELOPE_DIVISIONS - 1) );
        else
            m_Drag = clamp( ( ( e.pos.x + (m_divw / 2.0f) ) / box.size.x ) * (float)ENVELOPE_DIVISIONS, 0.0f, (float)ENVELOPE_DIVISIONS );

        m_bDrag = true;
    }
    else if( e.button == 1 )
    {
        if( m_EnvData[ m_currentChannel ].m_bGateMode )
            index = clamp( ( e.pos.x / box.size.x ) * (float)ENVELOPE_DIVISIONS, 0.0f, (float)(ENVELOPE_DIVISIONS - 1) );
        else
            index = clamp( ( ( e.pos.x + (m_divw / 2.0f) ) / box.size.x ) * (float)ENVELOPE_DIVISIONS, 0.0f, (float)ENVELOPE_DIVISIONS );

        m_EnvData[ m_currentChannel ].m_HandleVal[ index ] = 0.0f;
    }
}

//-----------------------------------------------------
// Procedure:   onDragStart
//-----------------------------------------------------
void Widget_EnvelopeEdit::onDragStart(EventDragStart &e)
{
    e.consumed = true;
    //windowCursorLock();
}

//-----------------------------------------------------
// Procedure:   onDragEnd
//-----------------------------------------------------
void Widget_EnvelopeEdit::onDragEnd(EventDragEnd &e)
{
    e.consumed = true;
     windowCursorUnlock();
     m_Drag = -1;
}

//-----------------------------------------------------
// Procedure:   onMouseMove
//-----------------------------------------------------
void Widget_EnvelopeEdit::onMouseMove( EventMouseMove &e )
{
    OpaqueWidget::onMouseMove( e );

    if( m_bDrag && m_bDraw )
    {
        m_Drawy = e.pos.y;

        if( m_EnvData[ m_currentChannel ].m_bGateMode )
            m_Drag = clamp( ( e.pos.x / box.size.x ) * (float)ENVELOPE_DIVISIONS, 0.0f, (float)(ENVELOPE_DIVISIONS - 1) );
        else
            m_Drag = clamp( ( ( e.pos.x + (m_divw / 2.0f) ) / box.size.x ) * (float)ENVELOPE_DIVISIONS, 0.0f, (float)ENVELOPE_DIVISIONS );
    }
}

//-----------------------------------------------------
// Procedure:   onDragMove
//-----------------------------------------------------
void Widget_EnvelopeEdit::onDragMove(EventDragMove &e)
{
    int h, i;
    float fband;
    float delta = 0.001f;

    e.consumed = true;

    if( !m_bInitialized || !m_bDrag )
        return;

    if( !m_bDraw )
    {
	    // Drag slower if Mod is held
	    if (windowIsModPressed())
		    delta = 0.00001f;

	    m_EnvData[ m_currentChannel ].m_HandleVal[ m_Drag ] -= delta * e.mouseRel.y;
	    m_EnvData[ m_currentChannel ].m_HandleVal[ m_Drag ] = clamp( m_EnvData[ m_currentChannel ].m_HandleVal[ m_Drag ], 0.0f, 1.0f );

        if( m_pCallback )
            m_pCallback( m_pClass, m_EnvData[ m_currentChannel ].getActualVal( m_EnvData[ m_currentChannel ].m_HandleVal[ m_Drag ] ) );

        if( m_fband > 0.0001 )
        {
            fband = m_fband;

            for( h = -1; h > -4; h -- )
            {
                i = m_Drag + h;

                if( i < 0 )
                    break;

                m_EnvData[ m_currentChannel ].m_HandleVal[ i ] -= (delta * e.mouseRel.y) * fband;
                m_EnvData[ m_currentChannel ].m_HandleVal[ i ] = clamp( m_EnvData[ m_currentChannel ].m_HandleVal[ i ], 0.0f, 1.0f );

                fband *= 0.6f;
            }

            fband = m_fband;

            for( h = 1; h < 4; h ++ )
            {
                i = m_Drag + h;

                if( i > ENVELOPE_DIVISIONS )
                    break;

                m_EnvData[ m_currentChannel ].m_HandleVal[ i ] -= (delta * e.mouseRel.y) * fband;
                m_EnvData[ m_currentChannel ].m_HandleVal[ i ] = clamp( m_EnvData[ m_currentChannel ].m_HandleVal[ i ], 0.0f, 1.0f );

                fband *= 0.6f;
            }

            recalcLine( -1, m_Drag );
        }
        else
        {
            recalcLine( m_currentChannel, m_Drag );
        }
    }
    else
    {
    	m_EnvData[ m_currentChannel ].m_HandleVal[ m_Drag ] = 1.0 - ( m_Drawy / box.size.y );
        m_EnvData[ m_currentChannel ].m_HandleVal[ m_Drag ] = clamp( m_EnvData[ m_currentChannel ].m_HandleVal[ m_Drag ], 0.0f, 1.0f );

        if( m_pCallback )
            m_pCallback( m_pClass, m_EnvData[ m_currentChannel ].getActualVal( m_EnvData[ m_currentChannel ].m_HandleVal[ m_Drag ] ) );

        recalcLine( m_currentChannel, m_Drag );
    }
}

//-----------------------------------------------------
// Procedure:   setBeatLen
//-----------------------------------------------------
void Widget_EnvelopeEdit::setBeatLen( int len )
{
    m_BeatLen = len;
    m_bClkd = true;

    if( m_BeatLen <= 0 )
        return;

    for( int i = 0; i < MAX_ENVELOPE_CHANNELS; i++ )
    {
        switch( m_TimeDiv[ i ] )
        {
        case TIME_64th:
        	m_EnvData[ i ].m_Clock.syncInc = ( ( engineGetSampleRate() / (float)m_BeatLen ) * 16.0 ) / 16.0;
            break;
        case TIME_32nd:
        	m_EnvData[ i ].m_Clock.syncInc = ( ( engineGetSampleRate() / (float)m_BeatLen ) * 8.0 ) / 16.0;
            break;
        case TIME_16th:
        	m_EnvData[ i ].m_Clock.syncInc = ( ( engineGetSampleRate() / (float)m_BeatLen ) * 4.0 ) / 16.0;
            break;
        case TIME_8th:
        	m_EnvData[ i ].m_Clock.syncInc = ( ( engineGetSampleRate() / (float)m_BeatLen ) * 2.0 ) / 16.0;
            break;
        case TIME_4tr:
        	m_EnvData[ i ].m_Clock.syncInc = ( ( engineGetSampleRate() / (float)m_BeatLen ) * 1.0 ) / 16.0;
            break;
        case TIME_Bar:
        	m_EnvData[ i ].m_Clock.syncInc = ( ( engineGetSampleRate() / (float)m_BeatLen ) * 0.25 ) / 16.0;
            break;
        }
    }
}

//-----------------------------------------------------
// Procedure:   procStep
//-----------------------------------------------------
float Widget_EnvelopeEdit::procStep( int ch, bool bTrig, bool bHold )
{
    if( ( m_bClkReset || bTrig ) && !bHold )
    {
        if( m_EnvData[ ch ].m_Mode == EnvelopeData::MODE_REVERSE )
        	m_EnvData[ ch ].m_Clock.fpos = engineGetSampleRate();
        else
        	m_EnvData[ ch ].m_Clock.fpos = 0;
    }

    return m_EnvData[ ch ].procStep( bTrig, bHold );
}
