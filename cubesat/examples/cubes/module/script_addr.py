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
            n.write(b"ADDR\n")
            time.sleep(1)
            for n in testbed.activeNode:
                n.flush()

    logger.info("Fine script")
