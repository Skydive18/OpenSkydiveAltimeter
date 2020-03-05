using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace PegasusLogbookConverter.Logbook
{
    public class Jump
    {
        /// <summary>
        /// Date and time of a jump
        /// </summary>
        public DateTime JumpDateTime { get; set; }

        public Dropzone Dropzone { get; set; }

        public int ExitAltitude { get; set; }

        public int DeployAltitude { get; set; }

        public int CanopyAltitude { get; set; }

        public int FreefallTime { get; set; }

        public int MaxFreefallSpeedMs { get; set; }

        public int AverageFreefallSpeedMs
            => FreefallTime > 0 ? (ExitAltitude - DeployAltitude) / FreefallTime : -1;

        public Array Points { get; set; } = Array.CreateInstance(typeof(int), 0);
    }
}
