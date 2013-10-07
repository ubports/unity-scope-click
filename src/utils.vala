/*
 * Copyright (C) 2010-2013 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Michal Hruby <michal.hruby@canonical.com>
 *
 */

namespace Utils
{
  private static Dee.ICUTermFilter icu_filter;

  public Dee.Index prepare_index (Dee.Model model,
                                  uint sort_column,
                                  owned Dee.ModelReaderFunc reader_func,
                                  out Dee.Analyzer out_analyzer)
  {
    // reuse the icu_filter
    if (icu_filter == null)
    {
      icu_filter = new Dee.ICUTermFilter.ascii_folder ();
    }

    var sort_filter = Dee.Filter.new_collator (sort_column);
    var filter_model = new Dee.FilterModel (model, sort_filter);

    var analyzer = new Dee.TextAnalyzer ();
    analyzer.add_term_filter ((terms_in, terms_out) =>
    {
      for (uint i = 0; i < terms_in.num_terms (); i++)
      {
        unowned string term = terms_in.get_term (i);
        var folded = icu_filter.apply (term);
        terms_out.add_term (term);
        if (folded != term) terms_out.add_term (folded);
      }
    });
    out_analyzer = analyzer;

    var reader = Dee.ModelReader.new ((owned) reader_func);
    return new Dee.TreeIndex (filter_model, analyzer, reader);
  }

  public SList<Dee.ModelIter> search_index (Dee.Index index,
                                            Dee.Analyzer analyzer,
                                            string query)
  {
    if (query.strip ().length == 0)
    {
      var model = index.get_model ();
      var iter = model.get_first_iter ();
      var end_iter = model.get_last_iter ();

      var result = new SList<Dee.ModelIter> ();
      while (iter != end_iter)
      {
        result.prepend (iter);
        iter = model.next (iter);
      }
      result.reverse ();
      return result;
    }

    var term_list = Object.new (typeof (Dee.TermList)) as Dee.TermList;
    analyzer.tokenize (query, term_list);
    var matches = new Sequence<Dee.ModelIter> ();

    uint num_terms = term_list.num_terms ();
    for (uint i = 0; i < num_terms; i++)
    {
      var rs = index.lookup (term_list.get_term (i),
                             i < num_terms - 1 ? Dee.TermMatchFlag.EXACT : Dee.TermMatchFlag.PREFIX);

      bool first_pass = i == 0;
      CompareDataFunc<Dee.ModelIter> cmp_func = (a, b) =>
      {
        return a == b ? 0 : ((void*) a > (void*) b ? 1 : -1);
      };
      // intersect the results (cause we want to AND the terms)
      var remaining = new Sequence<Dee.ModelIter> ();
      foreach (var item in rs)
      {
        if (first_pass)
          matches.insert_sorted (item, cmp_func);
        else if (matches.lookup (item, cmp_func) != null)
          remaining.insert_sorted (item, cmp_func);
      }
      if (!first_pass) matches = (owned) remaining;
      // final result set empty already?
      if (matches.get_begin_iter () == matches.get_end_iter ()) break;
    }

    var result = new SList<Dee.ModelIter> ();
    var iter = matches.get_begin_iter ();
    var end_iter = matches.get_end_iter ();
    while (iter != end_iter)
    {
      result.prepend (iter.get ());
      iter = iter.next ();
    }

    result.reverse ();
    return result;
  }
}
