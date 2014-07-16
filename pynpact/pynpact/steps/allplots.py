from __future__ import absolute_import
import logging
import os.path
import sys
import math
from subprocess import PIPE

from pynpact.steps import BaseStep, extract, nprofile
from pynpact import binfile
from pynpact import capproc
from pynpact.util import Hasher, reducedict, mkstemp_overwrite, which, delay


logger = logging.getLogger('pynpact.steps.allplots')
statuslog = logging.getLogger('pynpact.statuslog')


BIN = binfile('Allplots')

KEYS = ['first_page_title', 'following_page_title',
        'length', 'start_base', 'end_base', 'period', 'bp_per_page',
        'nucleotides', 'alternate_colors', 'basename']
FILE_KEYS = ['File_of_unbiased_CDSs',
             'File_of_conserved_CDSs',
             'File_of_new_CDSs',
             'File_of_published_rejected_CDSs',               #switched with "file_of_potential_new_CDs"
             'File_of_stretches_where_CG_is_asymmetric',
             'File_of_published_accepted_CDSs',
             'File_of_potential_new_CDSs',                    #switched with "file_of_published_rejected_CDs"
             'File_of_blocks_from_new_ORFs_as_cds',
             'File_of_blocks_from_annotated_genes_as_cds',
             'File_of_GeneMark_regions',
             'File_of_G+C_coding_potential_regions',
             'File_of_met_positions (e.g.:D 432)',
             'File_of_stop_positions (e.g.:D 432)',
             'File_of_tatabox_positions (e.g.:105.73 D 432 TATAAAAG)',
             'File_of_capbox_positions',
             'File_of_ccaatbox_positions',
             'File_of_gcbox_positions',
             'File_of_kozak_positions',
             'File_of_palindrom_positions_and_size',
             'File_list_of_nucleotides_in_200bp windows',
             'File_list_of_nucleotides_in_100bp windows']


class AllplotsStep(BaseStep):
    def dependencies(self):
        # If extract is to be run, kick it off while we have all the
        # configuration
        promises = []
        if self.config.get('run_extract'):
            filename = extract.ExtractStep.fromstep(self).enqueue()
            self.config['File_of_published_accepted_CDSs'] = filename
            promises.append(filename)

        filename = nprofile.NprofileStep.fromstep(self).enqueue()
        self.config['File_list_of_nucleotides_in_200bp windows'] = filename
        promises.append(filename)
        return promises

    def enqueue(self):
        h = Hasher()
        promises = self.dependencies()

        # Strip down to the config for this task only
        config = reducedict(self.config, KEYS + FILE_KEYS)

        bp_per_page = config['bp_per_page']
        start_base = config.pop('start_base')
        end_base = config.pop('end_base')

        page_count = math.ceil(float(end_base - start_base) / bp_per_page)
        page_num = 1  # page number offset
        filenames = []
        # per-page loop
        while start_base < end_base:
            config['start_base'] = start_base
            if start_base + bp_per_page < end_base:
                config['end_base'] = start_base + bp_per_page
            else:
                config['end_base'] = end_base
            h = Hasher().hashfiletime(BIN).hashdict(config)
            psname = self.derive_filename(h.hexdigest(), 'ps')
            task = delay(_ap)(psname, config, page_num, page_count)
            filenames.append(
                self.executor.enqueue(task, tid=psname, after=promises))
            page_num += 1
            start_base += bp_per_page

        return filenames


def _ap(psname, pconfig, page_num, page_count):
    statuslog.info(
        "Generating %d pages of graphical output: %2d%%",
        page_count, round(100.0 * page_num / page_count))

    cmd = [BIN, "-q", "--stdin"]
    if pconfig.get('alternate_colors'):
        cmd.append("-C")

    # add the rest of the required args
    cmd += [pconfig['start_base'],
            pconfig['bp_per_page'],
            # TODO: move these into config
            5,     # lines on a page
            1000,  # Number of subdivisions
            pconfig['period'],
            pconfig['end_base']]

    with mkstemp_overwrite(psname) as out:
        with capproc.guardPopen(
                cmd, stdin=PIPE, stdout=out, stderr=False,
                logger=logger) as ap:
            # write the allplots.def file information through stdin.
            write_allplots_def(ap.stdin, pconfig, page_num)
            ap.stdin.close()
            ap.wait()
    return psname


def write_allplots_def(out, pconfig, page_num):
    def wl(line):
        "helper function for writing a line to the allplots file."
        if line:
            out.write(line)
        out.write('\n')

    # NB: the "Plot Title" is disregarded, but that line
    # should also contain the total number of bases
    wl("%s %d" % ("FOOBAR", pconfig['length']))

    # Nucleotide(s)_plotted (e.g.: C+G)
    wl('+'.join(pconfig['nucleotides']))
    # First-Page title
    wl(pconfig['first_page_title'].format(page_num))
    # Title of following pages
    wl(pconfig['following_page_title'].format(page_num))
    # write the rest of the filenames that allplots might read from
    for k in FILE_KEYS:
        # allplots doesn't like blank lines so make sure there is at
        # least a dummy value on every line.
        wl(pconfig.get(k, "None"))


class CombinePsFilesStep(BaseStep):
    def enqueue(self):
        psnames = AllplotsStep.fromstep(self).enqueue()
        combined_ps_name = self.derive_filename(
            self.config['filename'],
            Hasher.hashlist(psnames).hexdigest(),
            'ps')
        self.executor.enqueue(
            delay(_combine_ps_files)(combined_ps_name, psnames),
            tid=combined_ps_name,
            after=psnames)


def _combine_ps_files(combined_ps_name, psnames):
    # While combining, insert the special markers so that
    # it will appear correctly as many pages.
    statuslog.info("Combining pages into output.")
    with mkstemp_overwrite(combined_ps_name) as psout:
        psout.write("%!PS-Adobe-2.0\n\n")
        psout.write("%%Pages: {0}\n\n".format(len(psnames)))
        idx = 1
        for psf in psnames:
            psout.write("%%Page: {0}\n".format(idx))
            with open(psf, 'r') as infile:
                infile.readline()
                infile.readline()
                psout.write(infile.readline())
                for l in infile:
                    psout.write(l)
            idx += 1
    return combined_ps_name


def Ps2PdfStep(BaseStep):
    def enqueue(self):
        combined_ps_name = CombinePsFilesStep.fromstep(self).enqueue()
        ps2pdf = which('ps2pdf')
        if ps2pdf:
            pdf_filename = self.replace_ext(combined_ps_name, 'pdf')
            self.executor.enqueue(
                delay(_ps2pdf)(combined_ps_name, pdf_filename),
                tid=pdf_filename,
                after=combined_ps_name)
            return pdf_filename
        else:
            logger.warn("Skipping ps2pdf translation, program not found.")
            return combined_ps_name


def _ps2pdf(ps_filename, pdf_filename):
    ps2pdf = which('ps2pdf')
    with mkstemp_overwrite(pdf_filename) as out:
        statuslog.info("Converting to PDF")
        cmd = [ps2pdf, ps_filename, '-']
        capproc.capturedCall(
            cmd, stdout=out, logger=logger, check=True)
        statuslog.info("Finished PDF conversion")
    return pdf_filename
