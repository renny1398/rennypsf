#pragma once
#include <cstdint>

namespace snesapu {

const uint32_t APU_CLK = 24576000;  //!< Number of clock cycles per second

/*!
 * \brief Disable DSP data register
 * \details Writes to the DSP data register (0F3h) will have no effect
 */
const uint32_t SPC_NODSP = 0x08;

/*!
 * \brief Trace execution
 * \details To step through each instruction.
 *          This flag will cause the debug vector to be called before executing the current instruction. <br/>
 *          This flag only works if spc700.cc is compiled with the SA_DEBUG option enabled (see apu.h).
 */
const uint32_t SPC_TRACE = 0x10;

/*!
 * \brief Return immediately from EmuSPC
 * \details This flag only works when used with SPC_TRACE.
 *          Once the debug vector returns, the CPU registers are updated then EmuSPC is terminated before the next instruction is executed. <br/>
 *          This only returns from EmuSPC, not from EmuAPU.  If you need to terminate DSP emulation as well, specify SPC_HALT in addition to SPC_RETURN.
 */
const uint32_t SPC_RETURN = 0x01;

//Halt APU:
//
//Subsequent calls to EmuSPC will return that cycles were emulated, but nothing will have happened.
//
//
/*!
 * \brief Halt APU
 * \details Subsequent calls to EmuSPC will return that cycles were emulated, but nothing will have happened. <br?
 *          Once this flag has been set, the next call to EmuSPC will also disable the DSP emulator.
 *          Subsequent calls to EmuDSP will generate silence.
 *          This way emulation can be halted in the middle of EmuAPU without affecting the size of the output buffer.
 */
const uint32_t SPC_HALT = 0x02;

struct SPCFlags
{
  uint8_t c:1;  //!< Carry
  uint8_t z:1;  //!< Zero
  uint8_t i:1;  //!< Interrupts Enabled (not used in the SNES)
  uint8_t h:1;  //!< Half-Carry (auxiliary)
  uint8_t b:1;  //!< Software Break
  uint8_t p:1;  //!< Direct Page Selector
  uint8_t v:1;  //!< Overflow
  uint8_t n:1;  //!< Negative (sign)
};

/*!
 * \typedef SPCDebug
 * \brief SPC700 Debugging Routine
 * \details A prototype for a function that gets called for debugging purposes <br/>
 *          The paramaters passed in can be modified, and on return will be used to update the internal registers (except 'cnt'). <br/>
 * \note When modifying PC or SP, only the lower word needs to be modified; the upper word will be ignored.
 * \param [in] pc Pointer to current opcode (LOWORD = PC)
 * \param [in] ya YA
 * \param [in] x X
 * \param [in] psw PSW
 * \param [in] sp Pointer to current stack byte (LOWORD = SP)
 * \param [in] cnt Clock cycle counters (four counters, one in each byte) <br/>
 *             [0-1] 8kHz cycles left until counters 0 and 1 increase <br/>
 *             [2]   64kHz cycles left until counter 2 increases <br/>
 *             [3]   CPU cycles left until next 64kHz clock pulse
 */
typedef void (*SPCDebug)(volatile uint8_t *pc, volatile uint16_t ya, volatile uint8_t x, volatile SPCFlags psw, volatile uint8_t *sp, volatile uint32_t cnt);

class SNESAPU;

class SPC700 {

public:

  /*!
   * \brief  Initialize SPC700
   * \details This function is a remnant from the 16-bit assembly when dynamic code reallocation was used.
   *          Now it just initializes internal pointers.
   * \note Callers should use InitAPU instead
   * \par Destroys
   *        EAX
   */
  SPC700(SNESAPU* snesapu);

  /*!
   * \brief Reset SPC700
   * \details Clears all memory, resets the function registers, T64Cnt, and halt flag, and copies ROM into the IPL region.
   * \note Callers should use ResetAPU instead
   * \par Destroys
   *        EAX
   */
  void ResetSPC();

  /*!
   * \brief Set SPC700 Debugging Routine
   * \details Installs a vector that gets called before instruction execution for debugging purposes.
   * \note pTrace is always called when a STOP instruction is encountered, regardless of the options.
   *       The build option SA_DEBUG must be enabled for pTrace to be called under other circumstances
   * \param [in] pTrace Pointer to debugging vector
   *                    (NULL can be passed to disable the debug vector, -1 leaves the vector currently in place)
   * \param [in] opts SPC700 debugging flags (see SPC_??? defines, -1 leaves the current flags)
   * \return Previously installed vector
   */
  SPCDebug* SetSPCDbg(SPCDebug *pTrace, uint32_t opts);

  /*!
   * \brief Fix SPC700 After Loading SPC File
   * \details Loads timer steps with the values in the timer registers, resets the counters, sets up the in/out ports, and stores the registers.
   * \note Callers should use FixAPU instead
   * \param [in] pc SPC internal registers
   * \param [in] a SPC internal registers
   * \param [in] y SPC internal registers
   * \param [in] x SPC internal registers
   * \param [in] psw SPC internal registers
   * \param [in] sp SPC internal registers
   * \par Destroys
   *        EAX
   */
  void FixSPC(uint16_t pc, uint8_t a, uint8_t y, uint8_t x, uint8_t psw, uint8_t sp);

  /*!
   * \brief Get SPC700 Registers
   * \details Returns the registers stored in the CPU.
   * \param [out] pPC Pointer to Vars to store SPC internal registers
   * \param [out] pA Pointer to Vars to store SPC internal registers
   * \param [out] pY Pointer to Vars to store SPC internal registers
   * \param [out] pX Pointer to Vars to store SPC internal registers
   * \param [out] pPSW Pointer to Vars to store SPC internal registers
   * \param [out] pSP Pointer to Vars to store SPC internal registers
   * \par Destroys
   *        EAX
   */
  void GetSPCRegs(uint16_t *pPC, uint8_t *pA, uint8_t *pY, uint8_t *pX, uint8_t *pPSW, uint8_t *pSP);

  /*!
   * \brief Write to APU RAM
   * \details Writes a value to APU RAM.  Use this instead of writing to RAM directly so any necessary internal changes can be made.
   * \param [in] addr Address to write to (only the lower 16-bits are used)
   * \param [in] val Value to write
   * \par Destroys
   *        EAX
   */
  void SetAPURAM(uint32_t addr, uint8_t val);

  /*!
   * \brief Write to SPC700 Port
   * \details Writes a value to the SPC700 via the in ports.  Use this instead of writing to RAM directly.
   * \param [in] port Port on which to write (0-3)
   * \param [in] val Value to write
   * \par Destroys
   *        EAX
   */
  void InPort(uint8_t port, uint8_t val);

  /*!
   * \brief Emulate SPC700
   * \details Emulates the SPC700 for the number of clock cycles specified, or if the counter break option is enabled,
   *          until a counter is increased, whichever happens first.
   * \note Callers should use EmuAPU instead <br/>
   *       Passing values <= 0 will cause undeterminable results
   * \param [in] cyc Number of 24.576MHz clock cycles to execute (must be > 0)
   * \return Clock cycles left to execute (negative if more cycles than specified were emulated)
   */
  int32_t EmuSPC(int32_t cyc);

private:
  SNESAPU* const snesapu_;

private:
  uint8_t   extraRAM[64];  //!< RAM used for storage if ROM reading is enabled
  uint8_t   outPort[4];    //!< Four out ports
  uint32_t  t64Cnt;        //!< Counter increased every 64kHz
  uint32_t* pSPCReg;       //!< Pointer to SPC700 Register Buffer

private:
  uint8_t inPortCp[4];
  uint8_t outPortCp[4];
  uint8_t flushPort[4];
  uint8_t portMod;
  uint8_t tControl;

  uint8_t t0Step;
  uint8_t t1Step;
  uint8_t t2Step[2];

  uint32_t clkTotal;
  uint32_t clkExec;
  uint32_t clkLeft;
  uint32_t t8kHz;
  uint32_t t64kHz;
  uint32_t t64Cnt;
  uint32_t pSPCReg;

  uint32_t pOpFetch;
  uint32_t pDebug;
  uint32_t dbgOpt;

  uint32_t PSW[8];
  uint8_t* regPC;
  uint32_t regYA;
  uint8_t* regSP;
  uint32_t regX;

  uint32_t cbWrPort;
  uint32_t cbRdPort;
  uint32_t cbReset;

  uint32_t oldT64;

  uint32_t spcVarEP;
};

} // namespace snesapu
