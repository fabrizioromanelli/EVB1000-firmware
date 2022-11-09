import testbed.Testbed
import logging
import time
import random

logger = logging.getLogger(__name__)

def run_test(testbed):
    logger.info("Inizio script")

    time.sleep(random.random()*4)
    while True:
        for n in testbed.activeNode:
            n.flush()
            count = 0
            n.write(b"SEND 114A 0008 0123456789ABCDEF\n")
            #n.write(b"SEND FFFF 0008 0123456789ABCDEF\n")
            time.sleep(1)
            n.write(b"RNGE 114A\n")
            time.sleep(random.random()*4)
            for n in testbed.activeNode:
                n.flush()

    logger.info("Fine script")
