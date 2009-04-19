/*
 * This Java file has been generated by smidump 0.4.5. It
 * is intended to be edited by the application programmer and
 * to be used within a Java AgentX sub-agent environment.
 *
 * $Id: NlMatrixSDEntryImpl.java 4432 2006-05-29 16:21:11Z strauss $
 */

/**
    This class extends the Java AgentX (JAX) implementation of
    the table row nlMatrixSDEntry defined in RMON2-MIB.
 */

import jax.AgentXOID;
import jax.AgentXSetPhase;
import jax.AgentXResponsePDU;
import jax.AgentXEntry;

public class NlMatrixSDEntryImpl extends NlMatrixSDEntry
{

    // constructor
    public NlMatrixSDEntryImpl(int hlMatrixControlIndex,
                           long nlMatrixSDTimeMark,
                           int protocolDirLocalIndex,
                           byte[] nlMatrixSDSourceAddress,
                           byte[] nlMatrixSDDestAddress)
    {
        super(hlMatrixControlIndex,
                 nlMatrixSDTimeMark,
                 protocolDirLocalIndex,
                 nlMatrixSDSourceAddress,
                 nlMatrixSDDestAddress);
    }

    public long get_nlMatrixSDPkts()
    {
        return nlMatrixSDPkts;
    }

    public long get_nlMatrixSDOctets()
    {
        return nlMatrixSDOctets;
    }

    public long get_nlMatrixSDCreateTime()
    {
        return nlMatrixSDCreateTime;
    }

}
