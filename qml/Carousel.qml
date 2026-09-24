import QtQuick

// Direct port of app.py DrawingArea — single logic, no redesign
Item {
    id: root
    property var model
    property int countVisible: 7
    property string borderColor: "#b4befe"
    property int borderWidth: 2
    property int idleBorderWidth: 2
    property string idleBorderColor: "#585b70"
    property real panelHeight: 500
    property real shear: 0.3
    property real spacing: 4.0
    property real horizontalScale: 1.6
    property real verticalScale: 1.1

    signal wallpaperClicked(int idx)
    signal applyRequested(int idx)

    property real visualSelection: 0
    property real targetSelection: 0
    property real contentX: 0
    property real targetX: 0

    property real tileWidth: Math.max(1, width / countVisible - 10)
    property real step: tileWidth + spacing
    property real extraWidth: tileWidth * (horizontalScale - 1.0)
    property real margin: tileWidth * 0.25
    property int count: 0

    Connections {
        target: root.model
        function onCountChanged() { root.count = root.model ? root.model.count() : 0 }
    }
    onModelChanged: root.count = model ? model.count() : 0

    function setSelected(idx) {
        if (count === 0) return
        if (isSmallCount) {
            idx = Math.max(0, Math.min(count - 1, idx))
            targetSelection = idx
            ensureVisible(idx)
            startAnim()
            return
        }
        idx = ((idx % count) + count) % count
        let cur = ((Math.round(targetSelection) % count) + count) % count
        let delta = idx - cur
        if (delta > count/2) delta -= count
        if (delta < -count/2) delta += count
        // filter keystrokes re-invoke this with the current index: fully
        // settled means nothing to do, skip the restart so typing never
        // hitches an idle carousel
        if (delta === 0 && Math.abs(targetSelection - visualSelection) < 0.01
                && Math.abs(targetX - contentX) < 0.5) return
        targetSelection += delta
        ensureVisible(targetSelection)
        startAnim()
    }
    function next() {
        if (isSmallCount) { setSelected(Math.min(count - 1, Math.round(targetSelection) + 1)); return }
        setSelected(((Math.round(targetSelection)+1)%count+count)%count)
    }
    function prev() {
        if (isSmallCount) { setSelected(Math.max(0, Math.round(targetSelection) - 1)); return }
        setSelected(((Math.round(targetSelection)-1)%count+count)%count)
    }
    function jumpForward() {
        if (isSmallCount) { setSelected(Math.min(count - 1, Math.round(targetSelection) + countVisible)); return }
        let c=countVisible; let t=Math.round(targetSelection)+c; setSelected(((t%count)+count)%count)
    }
    function jumpBack() {
        if (isSmallCount) { setSelected(Math.max(0, Math.round(targetSelection) - countVisible)); return }
        let c=countVisible; let t=Math.round(targetSelection)-c; setSelected(((t%count)+count)%count)
    }

    function ensureVisible(idx) {
        let center = idx * step + tileWidth / 2
        let viewportCenter = width / 2
        let x = center - viewportCenter
        if (isSmallCount) {
            let minX = tileWidth/2 - width/2
            let maxX = (count - 1) * step + tileWidth/2 - width/2
            x = Math.max(minX, Math.min(maxX, x))
        }
        targetX = x
        startAnim()
    }
    function startAnim() { anim.restart() }

    function setSelectedImmediate(idx) {
        if (count===0) return
        if (isSmallCount) idx = Math.max(0, Math.min(count - 1, idx))
        else idx = ((idx%count)+count)%count
        visualSelection = idx
        targetSelection = idx
        contentX = idx * step + tileWidth/2 - width/2
        targetX = contentX
        anim.running = false
        pendingRecenter = false
        winCenter = -99999
        syncSlots()
    }
    Component.onCompleted: {
        root.count = model ? model.count() : 0
        syncSlots()
    }

    function recenter() {
        if (count <= 0 || width <= 100) return
        const idx = isSmallCount
            ? Math.max(0, Math.min(count - 1, Math.round(visualSelection)))
            : Math.round(visualSelection)
        contentX = idx*step + tileWidth/2 - width/2
        targetX = contentX
    }

    onCountChanged: {
        // first wallpapers arrive after the window is already laid out;
        // center on them instead of leaving the stack at x=0
        if (count > 0 && visualSelection === 0 && targetX === 0) recenter()
        syncSlots()
    }
    onVisibleRangeChanged: syncSlots()
    onWidthChanged: {
        // never cancel an in-flight step, but remember to re-center once
        // it settles, otherwise targetX keeps the pre-resize geometry
        if (anim.running) { pendingRecenter = true; return }
        recenter()
    }

    FrameAnimation {
        id: anim
        running: false
        onTriggered: {
            // single rate for position and scale: highlight no longer leads
            // the slide. f clamped both sides: first frame after idle is a
            // full step (no frozen start), lag spikes at most double it.
            let f = Math.min(Math.max(anim.frameTime, 1/60), 1/30) * 60
            let k = 1 - Math.pow(1 - 0.18, f)
            let dx = targetX - contentX
            let ds = targetSelection - visualSelection
            let doneX = Math.abs(dx) < 0.5
            let doneS = Math.abs(ds) < 0.01
            if (doneX) contentX = targetX; else contentX += dx * k
            if (doneS) visualSelection = targetSelection; else visualSelection += ds * k
            if (!isSmallCount && count>0 && Math.abs(visualSelection) >= count) {
                let off = Math.floor(visualSelection / count) * count
                visualSelection -= off; targetSelection -= off
                contentX -= off*step; targetX -= off*step
            }
            if (!isSmallCount) syncSlots()
            if (doneX && doneS) {
                running = false
                if (pendingRecenter) { pendingRecenter = false; recenter() }
            }
        }
    }
    readonly property int selectedIndex: ((Math.round(visualSelection)% (count||1))+(count||1))%(count||1)
    // where the carousel is heading (== selectedIndex once settled)
    readonly property int committedIndex: ((Math.round(targetSelection)% (count||1))+(count||1))%(count||1)

    WheelHandler {
        onWheel: (e) => {
            if (root.count === 0) return
            let d = Math.abs(e.angleDelta.y) >= Math.abs(e.angleDelta.x) ? e.angleDelta.y : e.angleDelta.x
            let nx = targetX - d * 0.8
            if (isSmallCount) {
                let minX = tileWidth/2 - width/2
                let maxX = (count - 1) * step + tileWidth/2 - width/2
                nx = Math.max(minX, Math.min(maxX, nx))
                let idx = Math.round((nx + width/2 - tileWidth/2) / step)
                idx = Math.max(0, Math.min(count - 1, idx))
                targetSelection = idx
            } else {
                let idx = Math.round((nx + width/2 - tileWidth/2) / step)
                idx = ((idx % count) + count) % count
                let cur = ((Math.round(targetSelection) % count) + count) % count
                let delta = idx - cur
                if (delta > count/2) delta -= count
                if (delta < -count/2) delta += count
                targetSelection += delta
            }
            targetX = nx
            startAnim()
        }
    }
    MouseArea {
        id: dragArea
        anchors.fill: parent
        property real startX
        property real startContentX
        property bool dragging:false
        onPressed: (m)=>{ startX=m.x; startContentX=targetX; dragging=false }
        onPositionChanged: (m)=>{
            let off=m.x-startX
            if(!dragging && Math.abs(off)>8) dragging=true
            if(dragging){
                let nx = startContentX-off
                if (isSmallCount) {
                    let minX = tileWidth/2 - width/2
                    let maxX = (count - 1) * step + tileWidth/2 - width/2
                    nx = Math.max(minX, Math.min(maxX, nx))
                }
                targetX=nx; contentX=targetX; anim.running=false
            }
        }
        onReleased: (m)=>{
            if (root.count === 0) { dragging=false; return }
            if(dragging){
                // snap the target; visualSelection lerps there so scale
                // travels together with position instead of popping
                let idx = Math.round((targetX + width/2 - tileWidth/2) / step)
                root.setSelected(idx)
            }
            dragging=false
        }
    }

    property int visibleRange: Math.floor(countVisible/2)+2
    property bool isSmallCount: root.count > 0 && root.count <= root.visibleRange*2+1

    // Delegate slots are pinned to absolute vIdx values instead of being
    // derived from the rounded center. Deriving them (center-range+index)
    // shifted every slot on each half-step, so every pooled Image changed
    // source in the same frame and retainWhileLoading painted the previous
    // row's wallpaper in the middle of the slide. Here only the slot that
    // leaves the window is recycled, and that slot is always off-screen.
    property var slotVIdx: []
    property int winCenter: 0
    property bool pendingRecenter: false

    function syncSlots() {
        if (isSmallCount || count <= 0) { slotVIdx = []; winCenter = 0; return }
        const c = Math.round(visualSelection)
        if (c === winCenter && slotVIdx.length === visibleRange * 2 + 1) return
        const need = []
        for (let i = -visibleRange; i <= visibleRange; ++i) need.push(c + i)
        const kept = slotVIdx.filter(v => need.indexOf(v) >= 0)
        for (let i = 0; i < need.length; ++i)
            if (kept.indexOf(need[i]) < 0) kept.push(need[i])
        winCenter = c
        slotVIdx = kept.slice(0, visibleRange * 2 + 1)
    }

    Repeater {
        model: root.count>0 ? (root.isSmallCount ? root.count : root.visibleRange*2+1) : 0
        delegate: Item {
            required property int index
            property int vIdx: root.isSmallCount ? index
                                 : (root.slotVIdx[index] !== undefined ? root.slotVIdx[index] : index)
            property int realIdx: root.isSmallCount ? index : ((vIdx % root.count)+root.count)%root.count
            // these come from model; root.model.rev subscribes the binding
            // so dataChanged (new thumbs, refilter) re-evaluates it
            property string wallpaperPath: {
                if (!root.model) return ""
                root.model.rev
                return root.model.get_path_at(realIdx)
            }
            property string wallpaperName: {
                if (!root.model) return ""
                root.model.rev
                return root.model.get_name_at(realIdx)
            }
            property string thumbnailPath: {
                if (!root.model) return ""
                root.model.rev
                return root.model.get_thumb_at(realIdx)
            }

            property real dist: Math.abs(vIdx - root.visualSelection)
            property real progress: Math.max(0, 1 - dist)
            property real scaledW: root.tileWidth * (1 + (root.horizontalScale - 1.0) * progress)
            property real scaledH: root.panelHeight * (1 + (root.verticalScale - 1.0) * progress)
            property real shearOff: root.shear * scaledH
            property real baseX: vIdx * root.step - root.contentX
            property real compX: baseX + root.extraWidth/2
                                 * Math.max(-1, Math.min(1, vIdx - root.visualSelection))
            // Original: left = x + tile/2 - scaledW/2 - shear/2
            x: compX + root.tileWidth/2 - scaledW/2 - shearOff/2
            y: (root.height - scaledH)/2
            width: scaledW + shearOff
            height: scaledH
            z: isSelected ? 100 : 50 - Math.floor(dist)
            visible: !(x > root.width + root.margin || x + width < -root.margin)
            property bool isSelected: root.isSmallCount ? realIdx === root.selectedIndex : vIdx === Math.round(root.visualSelection)

            // Parallelogram clip: sheared box, image counter-sheared so the
            // picture itself stays upright (matches the original clip+paint).
            Item {
                id: tileClip
                width: scaledW
                height: scaledH
                clip: true
                transform: Matrix4x4 {
                    matrix: Qt.matrix4x4(1, -root.shear, 0, shearOff,
                                         0, 1, 0, 0,
                                         0, 0, 1, 0,
                                         0, 0, 0, 1)
                }

                Image {
                    source: thumbnailPath
                    width: scaledW + shearOff
                    height: scaledH
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    // a recycled tile must go blank rather than keep painting
                    // the wallpaper it used to show
                    retainWhileLoading: false
                    cache: true
                    // fixed cap: scale-dependent sourceSize would re-decode every frame
                    sourceSize.height: Math.round(root.panelHeight * root.verticalScale)
                    transform: Matrix4x4 {
                        matrix: Qt.matrix4x4(1, root.shear, 0, -shearOff,
                                             0, 1, 0, 0,
                                             0, 0, 1, 0,
                                             0, 0, 0, 1)
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    color: "black"
                    opacity: 0.16
                    visible: !isSelected
                }
            }

            // Border is stroked outside the clip, like the original reset_clip
            Rectangle {
                width: scaledW
                height: scaledH
                color: "transparent"
                border.width: isSelected ? root.borderWidth : root.idleBorderWidth
                border.color: isSelected ? root.borderColor : root.idleBorderColor
                visible: border.width > 0
                transform: Matrix4x4 {
                    matrix: Qt.matrix4x4(1, -root.shear, 0, shearOff,
                                         0, 1, 0, 0,
                                         0, 0, 1, 0,
                                         0, 0, 0, 1)
                }
            }

            // Click handling
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (isSelected) root.applyRequested(realIdx)
                    else root.setSelected(realIdx)
                    root.wallpaperClicked(realIdx)
                }
            }
        }
    }
}
