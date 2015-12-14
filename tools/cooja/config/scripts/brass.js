var motes = sim.getMotes();
var day = 1000 * 60 * 10;

for (var i = 0; i < motes.length; i++) {
    WAIT_UNTIL(msg.equals("waiting position"));
    log.log("i=" + i + "\n");
    log.log("x=" + mote.getInterfaces().getPosition().getXCoordinate() + "\n");
    log.log("y=" + mote.getInterfaces().getPosition().getYCoordinate() + "\n");
    write(mote, ""+mote.getInterfaces().getPosition().getXCoordinate());
    write(mote, ""+mote.getInterfaces().getPosition().getYCoordinate());
    YIELD();
}

while (true) {
    YIELD();
    log.log(time + " " + id + ": " + msg + "\n");
    if (sim.getSimulationTimeMillis() < day) continue;
    log.testOK();
}
