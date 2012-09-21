package net.freehal.core.util;

import java.io.File;

import net.freehal.core.grammar.AbstractGrammar;
import net.freehal.core.pos.AbstractTagger;

public class FreehalConfig {

	private static FreehalConfigImpl instance = null;

	private static AbstractTagger tagger = null;
	private static AbstractGrammar grammar = null;

	public static void set(FreehalConfigImpl instance) {
		FreehalConfig.instance = instance;
	}

	public static File getLanguageDirectory() {
		return instance.getLanguageDirectory();
	}

	public static String getLanguage() {
		return instance.getLanguage();
	}

	public static boolean isLanguage(final String otherLanguage) {
		return instance.getLanguage().equals(otherLanguage);
	}

	public static File getPath() {
		return instance.getPath();
	}

	public static AbstractTagger getTagger() {
		return tagger;
	}

	public static void setTagger(AbstractTagger tagger) {
		FreehalConfig.tagger = tagger;
	}

	public static AbstractGrammar getGrammar() {
		return grammar;
	}

	public static void setGrammar(AbstractGrammar grammar) {
		FreehalConfig.grammar = grammar;
	}
}