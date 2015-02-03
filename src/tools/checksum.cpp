/*
   checksum - A tool dumping MPLS structures

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/bswap.h"
#include "common/checksums/crc.h"
#include "common/command_line.h"
#include "common/endian.h"
#include "common/mm_io_x.h"
#include "common/strings/parsing.h"

class cli_options_c {
public:
  std::string m_file_name;
  mtx::checksum::algorithm_e m_algorithm;
  size_t m_chunk_size;
  uint64_t m_initial_value, m_xor_result;

  cli_options_c()
    : m_algorithm{mtx::checksum::adler32}
    , m_chunk_size{4096}
    , m_initial_value{}
    , m_xor_result{}
  {
  }
};

static void
show_help() {
  mxinfo("checksum [options] file_name\n"
         "\n"
         "Calculates a checksum of a file. Used for testing MKVToolNix' checksumming\n"
         "algorithms. The defaults are:\n"
         "- Algorithm: Adler32\n"
         "- Chunk size 4096\n"
         "- Initial vlaue: 0\n"
         "- XOR result with: 0\n"
         "\n"
         "Checksum options:\n"
         "\n"
         "  -a, --adler32          Use Adler32 (default algorithm)\n"
         "      --crc8-atm         Use CRC-8 ATM\n"
         "      --crc16-ansi       Use CRC-16 ANSI\n"
         "      --crc16-ccitt      Use CRC-16 CCITT\n"
         "  -c, --crc32-ieee       Use CRC-32 IEEE\n"
         "      --crc32-ieee-le    Use CRC-32 IEEE Little Endian\n"
         "  -m, --md5              Use MD5\n"
         "  --chunk-size size      Calculate in chunks of »size« bytes; 0 means all\n"
         "                         in one (default: 4096)\n"
         "  --initial-value value  Use this as the initial value for CRC algorithms\n"
         "                         (default: 0)\n"
         "  --xor-result value     XOR the result with this value for CRC algorithms\n"
         "                         (default: 0)\n"
         "\n"
         "General options:\n"
         "\n"
         "  -h, --help             This help text\n"
         "  -V, --version          Print version information\n");
  mxexit();
}

static void
show_version() {
  mxinfo("checksum v" VERSION "\n");
  mxexit();
}

static cli_options_c
parse_args(std::vector<std::string> &args) {
  auto options = cli_options_c{};

  for (auto current = args.begin(), end = args.end(); current != end; ++current) {
    auto arg      = *current;
    auto next     = current + 1;
    auto next_arg = next != end ? *next : "";

    if ((arg == "-h") || (arg == "--help"))
      show_help();

    else if ((arg == "-V") || (arg == "--version"))
      show_version();

    else if ((arg == "-a") || (arg == "--adler32"))
      options.m_algorithm = mtx::checksum::adler32;

    else if (arg == "--crc8-atm")
      options.m_algorithm = mtx::checksum::crc8_atm;

    else if (arg == "--crc16-ansi")
      options.m_algorithm = mtx::checksum::crc16_ansi;

    else if (arg == "--crc16-ccitt")
      options.m_algorithm = mtx::checksum::crc16_ccitt;

    else if ((arg == "-c") || (arg == "--crc32-ieee"))
      options.m_algorithm = mtx::checksum::crc32_ieee;

    else if (arg == "--crc32-ieee-le")
      options.m_algorithm = mtx::checksum::crc32_ieee_le;

    else if ((arg == "-m") || (arg == "--md5"))
      options.m_algorithm = mtx::checksum::md5;

    else if (arg == "--chunk-size") {
      if (next_arg.empty())
        mxerror(boost::format("Missing argument to %1%\n") % arg);

      if (!parse_number(next_arg, options.m_chunk_size))
        mxerror(boost::format("Invalid argument to %1%: %2%\n") % arg % next_arg);

      ++current;

    } else if (arg == "--initial-value") {
      if (next_arg.empty())
        mxerror(boost::format("Missing argument to %1%\n") % arg);

      if (!parse_number(next_arg, options.m_initial_value))
        mxerror(boost::format("Invalid argument to %1%: %2%\n") % arg % next_arg);

      ++current;

    } else if (arg == "--xor-result") {
      if (next_arg.empty())
        mxerror(boost::format("Missing argument to %1%\n") % arg);

      if (!parse_number(next_arg, options.m_xor_result))
        mxerror(boost::format("Invalid argument to %1%: %2%\n") % arg % next_arg);

      ++current;

    } else if (!options.m_file_name.empty())
      mxerror("More than one input file given\n");

    else
      options.m_file_name = arg;
  }

  if (options.m_file_name.empty())
    mxerror("No file name given\n");

  return options;
}

static void
parse_file(cli_options_c const &options) {
  auto in         = mm_file_io_c{options.m_file_name};
  auto file_size  = in.get_size();
  auto chunk_size = !options.m_chunk_size ? file_size : std::min<int64_t>(file_size, options.m_chunk_size);
  auto total_read = 0ll;
  auto buffer     = memory_c::alloc(chunk_size);
  auto worker     = mtx::checksum::for_algorithm(options.m_algorithm);
  auto crc_worker = dynamic_cast<mtx::checksum::crc_base_c *>(worker.get());

  if (crc_worker) {
    crc_worker->set_initial_value(options.m_initial_value);
    crc_worker->set_xor_result(options.m_xor_result);
  }

  while (total_read < file_size) {
    auto remaining = file_size - total_read;
    chunk_size     = std::min<int64_t>(chunk_size, remaining);
    auto num_read  = in.read(buffer, chunk_size);
    total_read    += num_read;

    if (num_read != chunk_size)
      mxerror("Could not read the file.\n");

    worker->add(buffer->get_buffer(), chunk_size);
  }

  worker->finish();

  auto result   = worker->get_result();
  auto ptr      = result->get_buffer();
  auto res_size = result->get_size();
  std::string output;

  for (auto idx = 0u; idx < res_size; idx++)
    output += (boost::format("%|1$02x|") % static_cast<unsigned int>(ptr[idx])).str();

  mxinfo(boost::format("%1%  %2%\n") % output % options.m_file_name);
}

int
main(int argc,
     char **argv) {
  mtx_common_init("checksum", argv[0]);

  auto args = command_line_utf8(argc, argv);
  while (handle_common_cli_args(args, "-r"))
    ;

  auto options = parse_args(args);

  try {
    parse_file(options);
  } catch (mtx::mm_io::exception &) {
    mxerror("File not found\n");
  }

  mxexit();
}
